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

#include <netlink/route/qdisc/netem.h>
#include <netlink/route/tc.h>

#include "tc.h"
#include "config.h"
#include "timing.h"

int emulate(int argc, char *argv[])
{
	int ret;

	struct nl_sock *sock;
	struct nl_dump_params dp_param = {
		.dp_type = NL_DUMP_STATS,
		.dp_fd = stdout
	};
	struct rtnl_link *link;
	struct rtnl_tc *qdisc_prio = NULL;
	struct rtnl_tc *qdisc_netem = NULL;
	struct rtnl_tc *cls_fw = NULL;
	
	struct tc_netem netem = {
		.limit = cfg.limit,
		.delay = 100000
	};

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
	ret = tc_prio(sock, link, &qdisc_prio);
	if (ret)
		error(-1, 0, "Failed to setup TC: prio qdisc: %s", nl_geterror(ret));

	ret = tc_classifier(sock, link, &cls_fw, cfg.mark, cfg.mask);
	if (ret)
		error(-1, 0, "Failed to setup TC: fw filter: %s", nl_geterror(ret));
	
	ret = tc_netem(sock, link, &qdisc_netem, &netem);
	if (ret)
		error(-1, 0, "Failed to setup TC: netem qdisc: %s", nl_geterror(ret));
	
	/* We will use the default normal distribution for now */
	rtnl_netem_set_delay_distribution(qdisc_netem, "normal");
	
	/* Start timer */
	struct itimerspec its = {
		.it_interval = time_from_double(1 / cfg.rate),
		.it_value = { 1, 0 }
	};

	int tfd = timerfd_create(CLOCK_REALTIME, 0);
	if (tfd < 0)
		error(-1, errno, "Failed to create timer");

	if (timerfd_settime(tfd, 0, &its, NULL))
		error(-1, errno, "Failed to start timer");
	
	char *line = NULL;
	size_t linelen = 0;
	
	unsigned run = 0;
	while (!cfg.limit || run < cfg.limit) {
		float rtt, mu, sigma;
		
		/* Show queuing discipline statistics */
		rtnl_tc_dump_stats(qdisc_netem, &dp_param);	

		/* Parse new data */
		if (feof(stdin) || getline(&line, &linelen, stdin) == -1)
			error(-1, errno, "Failed to read data from stdin");
		
		if (line[0] == '#')
			continue;
		
		if (sscanf(line, "%f %f %f ", &rtt, &mu, &sigma) != 3)
			error(-1, 0, "Invalid data format");
		
		/* Update the netem config according to the measurements */
		/* TODO: Add more characteristics */
		netem.delay = (mu / 2) * 1e6;
		netem.jitter = sigma * 1e6;
				
		ret = tc_netem(sock, link, &qdisc_netem, &netem);
		if (ret)
			error(-1, 0, "Failed to update TC: netem qdisc: %s", nl_geterror(ret));
		
		timerfd_wait(tfd);
		run++;
	}
	
	/* Shutdown */
	nl_close(sock);
	nl_socket_free(sock);
	
	return 0;
}