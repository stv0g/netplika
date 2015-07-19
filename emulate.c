/** Setup and update netem qdisc.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#include <error.h>

#include "tc.h"
#include "config.h"

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
		.delay = 1000000,
		.limit = cfg.limit
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

	ret = tc_netem(sock, link, &qdisc_netem, &netem);
	if (ret)
		error(-1, 0, "Failed to setup TC: netem qdisc: %s", nl_geterror(ret));

	ret = tc_classifier(sock, link, &cls_fw, cfg.mark, cfg.mask);
	if (ret)
		error(-1, 0, "Failed to setup TC: fw filter: %s", nl_geterror(ret));
	
	int run = 0;
	while (cfg.limit && run < cfg.limit) {
		rtnl_tc_dump_stats(qdisc_netem, &dp_param);
		sleep(1);
		
		netem.delay += 10000;
		ret = tc_netem(sock, link, &qdisc_netem, &netem);
		if (ret)
			error(-1, 0, "Failed to update TC: netem qdisc: %s", nl_geterror(ret));
		
		run++;
	}
	
	/* Shutdown */
	nl_close(sock);
	nl_socket_free(sock);
	
	return 0;
}