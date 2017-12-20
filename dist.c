/** Generate distribution tables for netem qdisc.
 *
 * Partially based on:
 *  - http://git.kernel.org/cgit/linux/kernel/git/shemminger/iproute2.git/tree/netem/maketable.c
 *  - https://github.com/thom311/libnl/blob/master/lib/route/qdisc/netem.c
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netlink/route/qdisc.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc/netem.h>

#include "netlink-private.h"
#include "dist-maketable.h"
#include "tc.h"
#include "config.h"

#define SCH_NETEM_ATTR_DIST 0x2000

/**
 * Set the delay distribution. Latency/jitter must be set before applying.
 * @arg qdisc Netem qdisc.
 * @return 0 on success, error code on failure.
 */
int rtnl_netem_set_delay_distribution_data(struct rtnl_qdisc *qdisc, short *data, size_t len) {
	struct rtnl_netem *netem;

	if (!(netem = rtnl_tc_data(TC_CAST(qdisc))))
		BUG();

	if (len > MAXDIST)
		return -NLE_INVAL;

	netem->qnm_dist.dist_data = (int16_t *) calloc(len, sizeof(int16_t));

	size_t i;
	for (i = 0; i < len; i++)
		netem->qnm_dist.dist_data[i] = data[i];

	netem->qnm_dist.dist_size = len;
	netem->qnm_mask |= SCH_NETEM_ATTR_DIST;

	return 0;
}

static short * dist_make(FILE *fp, double *mu, double *sigma, double *rho, int *cnt)
{
	double *measurements;
	int *table;
	short *inverse;
	int total;

	measurements = readdoubles(fp, cnt);
	if (*cnt <= 0)
		error(-1, 0, "Nothing much read!");

	for (int i = 0; i < *cnt; i++)
		measurements[i] *= cfg.scaling;

	arraystats(measurements, *cnt, mu, sigma, rho);

	table = makedist(measurements, *cnt, *mu, *sigma);
	cumulativedist(table, DISTTABLESIZE, &total);

	inverse = inverttable(table, TABLESIZE, DISTTABLESIZE, total);
	interpolatetable(inverse, TABLESIZE);

	free((void *) measurements);
	free((void *) table);

	return inverse;
}

static int dist_generate(int argc, char *argv[])
{
	FILE *fp;
	double mu, sigma, rho;
	int cnt;

	if (argc == 1) {
		if (!(fp = fopen(argv[0], "r")))
			error(-1, errno, "Failed to open file: %s", argv[0]);
	}
	else
		fp = stdin;

	short *inverse = dist_make(fp, &mu, &sigma, &rho, &cnt);
	if (!inverse)
		error(-1, 0, "Failed to generate distribution");

	char date[100], user[100], host[100];
    time_t now = time (0);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&now));
	gethostname(host, sizeof(host));
	getlogin_r(user, sizeof(user));

	printf("# This is the distribution table for the experimental distribution.\n");
	printf("#  Read %d values, mu %.6f, sigma %.6f, rho %.6f\n", cnt, mu, sigma, rho);
    printf("#  Generated %s, by %s on %s\n", date, user, host);
	printf("#\n");

	switch (cfg.format) {
		case FORMAT_TC:
			printtable(inverse, TABLESIZE);
			break;

		case FORMAT_VILLAS:
			printf("netem = {\n");
			printf("	delay        = %f\n", mu * 1e6);
			printf("	jitter       = %f\n", sigma * 1e6);
			printf("	distribution = [ %d", inverse[0]);

			for (int i = 1; i < TABLESIZE; i++)
				printf(", %d", inverse[i]);

			printf(" ]\n");
			printf("	loss         = 0,\n");
			printf("	duplicate    = 0,\n");
			printf("	corrupt      = 0\n");
			printf(" }\n");
			break;
	}

	return 0;
}

static int dist_load(int argc, char *argv[])
{
	FILE *fp;
	double mu, sigma, rho;
	int cnt;

	if (argc == 1) {
		if (!(fp = fopen(argv[0], "r")))
			error(-1, errno, "Failed to open file: %s", argv[0]);
	}
	else
		fp = stdin;

	short *inverse = dist_make(fp, &mu, &sigma, &rho, &cnt);
	if (!inverse)
		error(-1, 0, "Failed to generate distribution");

	int ret;

	struct nl_sock *sock;

	struct rtnl_link *link;
	struct rtnl_tc *qdisc_prio = NULL;
	struct rtnl_tc *qdisc_netem = NULL;
	struct rtnl_tc *cls_fw = NULL;

	/* Create connection to netlink */
	sock = nl_socket_alloc();
	nl_connect(sock, NETLINK_ROUTE);

	/* Get interface */
	link = tc_get_link(sock, cfg.dev);
	if (!link)
		error(-1, 0, "Interface does not exist: %s", cfg.dev);

	/* Reset TC subsystem */
	ret = tc_reset(sock, link);
	if (ret && ret != -NLE_OBJ_NOTFOUND)
		error(-1, 0, "Failed to reset TC: %s", nl_geterror(ret));

	/* Setup TC subsystem */
	if ((ret = tc_prio(sock, link, &qdisc_prio)))
		error(-1, 0, "Failed to setup TC: prio qdisc: %s", nl_geterror(ret));

	if ((ret = tc_classifier(sock, link, &cls_fw, cfg.mark, cfg.mask)))
		error(-1, 0, "Failed to setup TC: fw filter: %s", nl_geterror(ret));

	if ((ret = tc_netem(sock, link, &qdisc_netem)))
		error(-1, 0, "Failed to setup TC: netem qdisc: %s", nl_geterror(ret));

	/* We will use the default normal distribution for now */
	if (rtnl_netem_set_delay_distribution_data((struct rtnl_qdisc *) qdisc_netem, inverse, TABLESIZE))
		error(-1, 0, "Failed to set netem delay distrubtion: %s", nl_geterror(ret));

	rtnl_netem_set_delay((struct rtnl_qdisc *) qdisc_netem, mu);
	rtnl_netem_set_jitter((struct rtnl_qdisc *) qdisc_netem, sigma);

	nl_close(sock);
	nl_socket_free(sock);

	return 0;
}

int dist(int argc, char *argv[])
{
	char *subcmd = argv[0];

	if (argc != 1)
		error(-1, 0, "Missing sub-command");

	if      (!strcmp(subcmd, "generate"))
		return dist_generate(argc-1, argv+1);
	else if (!strcmp(subcmd, "load"))
		return dist_load(argc-1, argv+1);
	else
		return -1;
}
