/** Setup and update netem qdisc.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <time.h>

#include <sys/timerfd.h>

#include <netlink/route/qdisc/netem.h>
#include <netlink/route/tc.h>

#include "tc.h"
#include "config.h"
#include "timing.h"

enum input_fields {
	CURRENT_RTT,
	MEAN,
	SIGMA,
	GAP,
	LOSS_PROB,
	LOSS_CORR,
	REORDER_PROB,
	REORDER_CORR,
	CORRUPTION_PROB,
	CORRUPTION_CORR,
	DUPLICATION_PROB,
	DUPLICATION_CORR,
	MAXFIELDS
};

static int emulate_parse_line(char *line, struct rtnl_tc *tc)
{	
	double val;
	char *cur, *end = line;
	int i = 0;
	
	struct rtnl_qdisc *ne = (struct rtnl_qdisc *) tc;

	do {
		cur = end;
		val = strtod(cur, &end);
		
		switch (i) {	
			case CURRENT_RTT:
				rtnl_netem_set_delay(ne, val * 1e6 / 2);
				break; /* we approximate: delay = RTT / 2 */
			case MEAN:
				break; /* ignored */
			case SIGMA:
				rtnl_netem_set_jitter(ne, val * 1e6 + 1);
				break;
			case GAP:
				rtnl_netem_set_gap(ne, val);
				break;
			case LOSS_PROB:
				rtnl_netem_set_loss(ne, val);
				break;
			case LOSS_CORR:
				rtnl_netem_set_loss_correlation(ne, val);
				break;
			case REORDER_PROB:
				rtnl_netem_set_reorder_probability(ne, val);
				break;
			case REORDER_CORR:
				rtnl_netem_set_reorder_correlation(ne, val);
				break;
			case CORRUPTION_PROB:
				rtnl_netem_set_corruption_probability(ne, val);
				break;
			case CORRUPTION_CORR:
				rtnl_netem_set_corruption_correlation(ne, val);
				break;
			case DUPLICATION_PROB:
				rtnl_netem_set_duplicate(ne, val);
				break;
			case DUPLICATION_CORR:
				rtnl_netem_set_duplicate_correlation(ne, val);
				break;
		}
	} while (cur != end && ++i < MAXFIELDS);
	
	
	return (i >= 3) ? 0 : -1; /* we need at least 3 fields: rtt + jitter */
}

int emulate(int argc, char *argv[])
{
	int ret, tfd, run = 0;

	struct nl_sock *sock;
	
	struct rtnl_link *link;
	struct rtnl_tc *qdisc_prio = NULL;
	struct rtnl_tc *qdisc_netem = NULL;
	struct rtnl_tc *cls_fw = NULL;

	struct tc_statistics stats_netem;

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
	if (rtnl_netem_set_delay_distribution((struct rtnl_qdisc *) qdisc_netem, "normal"))
		error(-1, 0, "Failed to set netem delay distrubtion: %s", nl_geterror(ret));
	
	rtnl_netem_set_limit((struct rtnl_qdisc *) qdisc_netem, 0);
	
	/* Start timer */
	if ((tfd = timerfd_init(cfg.rate)) < 0)
		error(-1, errno, "Failed to initilize timer");
	
	char *line = NULL;
	size_t linelen = 0;
	ssize_t len;
	
	do {
#if 0
		struct nl_dump_params dp_param = {
			.dp_type = NL_DUMP_DETAILS,
			.dp_fd = stdout
		};
		
		nl_object_dump((struct nl_object *) qdisc_netem, &dp_param);
		nl_object_dump((struct nl_object *) qdisc_prio, &dp_param);	
		nl_object_dump((struct nl_object *) cls_fw, &dp_param);	
#endif

		tc_print_netem(qdisc_netem);
		tc_print_stats(&stats_netem);

next_line:	len = getline(&line, &linelen, stdin);
		if (len < 0 && errno == ENOENT)
			break; /* EOF => quit */
		else if (len < 0)
			error(-1, errno, "Failed to read data from stdin");
		
		if (line[0] == '#' || line[0] == '\r' || line[0] == '\n')
			goto next_line;
		
		if (emulate_parse_line(line, qdisc_netem))
			error(-1, 0, "Failed to parse stdin");

		ret = tc_netem(sock, link, &qdisc_netem);
		if (ret)
			error(-1, 0, "Failed to update TC: netem qdisc: %s", nl_geterror(ret));
		
		run = timerfd_wait(tfd);
	} while (!cfg.limit || run < cfg.limit);
	
	/* Shutdown */
	free(line);

	nl_close(sock);
	nl_socket_free(sock);

	return 0;
}
