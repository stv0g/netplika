/** Trafic Controller (TC) related functions
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#ifndef _TC_H_
#define _TC_H_

#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/classifier.h>

struct tc_netem {
	int limit;
	int gap;
	int reorder_prob;
	int reorder_corr;
	int corruption_prob;
	int corruption_corr;
	int loss_prob;
	int loss_corr;
	int duplication_prob;
	int duplication_corr;
	int jitter;
	int delay;
	int delay_corr;
	char *delay_distr;
};

/*struct tc_stats {
	uint64_t packets;	// Number of packets seen.
	uint64_t bytes;		// Total bytes seen.
	uint64_t rate_bps;	// Current bits/s (rate estimator)
	uint64_t rate_pps;	// Current packet/s (rate estimator)
	uint64_t qlen;		// Current queue length.
	uint64_t backlog;	// Current backlog length.
	uint64_t drops;		// Total number of packets dropped.
	uint64_t requeues;	// Total number of requeues.
	uint64_t overlimits;	// Total number of overlimits.	
};*/

struct rtnl_link * tc_get_link(struct nl_sock *sock, const char *dev);

int tc_prio(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc);

int tc_netem(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc, struct tc_netem *ne);

int tc_classifier(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc, int mark, int mask);

int tc_reset(struct nl_sock *sock, struct rtnl_link *link);

int tc_get_stats(struct nl_sock *sock, struct rtnl_tc *tc, struct tc_stats *stats);

int tc_print_stats(struct tc_stats *stats);

#endif