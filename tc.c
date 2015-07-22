/** Trafic Controller (TC) related functions
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 1
#include <netdb.h>

#include <netlink/route/qdisc/netem.h>
#include <netlink/route/qdisc/prio.h>
#include <netlink/route/cls/fw.h>
#include <netlink/fib_lookup/request.h>
#include <netlink/fib_lookup/lookup.h>

#include <linux/if_ether.h>

#include "tc.h"

struct rtnl_link * tc_get_link(struct nl_sock *sock, const char *dev)
{
	struct nl_cache *cache;
	struct rtnl_link *link;

	rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
	link = rtnl_link_get_by_name(cache, "eth0");
	nl_cache_put(cache);

	return link;
}

int tc_prio(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc)
{
	/* This is the default priomap +1 for every entry.
	 * The first band (value == 0) with the highest priority is reserved for the netem traffic */
	uint8_t map[] = { 2, 3, 3, 3, 2, 3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0 };

	struct rtnl_qdisc *q = rtnl_qdisc_alloc();

	rtnl_tc_set_link(TC_CAST(q), link);
	rtnl_tc_set_parent(TC_CAST(q), TC_H_ROOT);
	rtnl_tc_set_handle(TC_CAST(q), TC_HANDLE(1, 0));
	rtnl_tc_set_kind(TC_CAST(q), "prio"); 

	rtnl_qdisc_prio_set_bands(q, 3+1);
	rtnl_qdisc_prio_set_priomap(q, map, 7);

	int ret = rtnl_qdisc_add(sock, q, NLM_F_CREATE);

	*tc = TC_CAST(q);
	
	return ret;
}

int tc_netem(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc)
{
	struct rtnl_qdisc *q;
	
	if (*tc == NULL) {
		q = rtnl_qdisc_alloc();

		rtnl_tc_set_link(TC_CAST(q), link);
		rtnl_tc_set_parent(TC_CAST(q), TC_HANDLE(1, 1));
		rtnl_tc_set_handle(TC_CAST(q), TC_HANDLE(2, 0));
		rtnl_tc_set_kind(TC_CAST(q), "netem");
	}
	else
		q = (struct rtnl_qdisc *) (*tc);

	int ret = rtnl_qdisc_add(sock, q, NLM_F_CREATE);

	*tc = TC_CAST(q);
	
	return ret;
}

int tc_classifier(struct nl_sock *sock, struct rtnl_link *link, struct rtnl_tc **tc, int mark, int mask)
{
	struct rtnl_cls *c = rtnl_cls_alloc();

	rtnl_tc_set_link(TC_CAST(c), link);
	rtnl_tc_set_handle(TC_CAST(c), mark);
	rtnl_tc_set_kind(TC_CAST(c), "fw"); 

	rtnl_cls_set_protocol(c, ETH_P_ALL);
	
	rtnl_fw_set_classid(c, TC_HANDLE(1, 1));
	rtnl_fw_set_mask(c, mask);

	int ret = rtnl_cls_add(sock, c, NLM_F_CREATE);

	*tc = TC_CAST(c);
	
	return ret;
}

int tc_reset(struct nl_sock *sock, struct rtnl_link *link)
{
	struct rtnl_qdisc *q = rtnl_qdisc_alloc();

	/* Restore default qdisc by deleting the root qdisc (see tc-pfifo_fast(8)) */
	rtnl_tc_set_link(TC_CAST(q), link);
	rtnl_tc_set_parent(TC_CAST(q), TC_H_ROOT);
	
	int ret = rtnl_qdisc_delete(sock, q); 
	rtnl_qdisc_put(q);
	
	return ret;
}

int tc_get_stats(struct nl_sock *sock, struct rtnl_tc *tc, struct tc_stats *stats)
{
	uint64_t *counters = (uint64_t *) stats;
	
	struct nl_cache *cache;
	
	rtnl_qdisc_alloc_cache(sock, &cache);
	
	for (int i = 0; i <= RTNL_TC_STATS_MAX; i++)
		counters[i] = rtnl_tc_get_stat(tc, i);
			
	nl_cache_free(cache);

	return 0;
}

int tc_print_stats(struct tc_statistics *stats)
{
	printf("packets %u bytes %u\n", stats->packets, stats->bytes);
}

int tc_print_netem(struct rtnl_tc *tc)
{
	struct rtnl_qdisc *ne = (struct rtnl_qdisc *) tc;
	
	if (rtnl_netem_get_limit(ne) > 0)
		printf("limit %upkts", rtnl_netem_get_limit(ne));

	if (rtnl_netem_get_delay(ne) > 0) {
		printf("delay %fms ", rtnl_netem_get_delay(ne) / 1000.0);
		
		if (rtnl_netem_get_jitter(ne) > 0) {
			printf("jitter %fms ", rtnl_netem_get_jitter(ne) / 1000.0);
			
			if (rtnl_netem_get_delay_correlation(ne) > 0)
				printf("%u%% ", rtnl_netem_get_delay_correlation(ne));
		}
	}
	
	if (rtnl_netem_get_loss(ne) > 0) {
		printf("loss %u%% ", rtnl_netem_get_loss(ne));
	
		if (rtnl_netem_get_loss_correlation(ne) > 0)
			printf("%u%% ", rtnl_netem_get_loss_correlation(ne));
	}
	
	if (rtnl_netem_get_reorder_probability(ne) > 0) {
		printf(" reorder%u%% ", rtnl_netem_get_reorder_probability(ne));
	
		if (rtnl_netem_get_reorder_correlation(ne) > 0)
			printf("%u%% ", rtnl_netem_get_reorder_correlation(ne));
	}
	
	if (rtnl_netem_get_corruption_probability(ne) > 0) {
		printf("corruption %u%% ", rtnl_netem_get_corruption_probability(ne));
	
		if (rtnl_netem_get_corruption_correlation(ne) > 0)
			printf("%u%% ", rtnl_netem_get_corruption_correlation(ne));
	}
	
	if (rtnl_netem_get_duplicate(ne) > 0) {
		printf("duplication %u%% ", rtnl_netem_get_duplicate(ne));
	
		if (rtnl_netem_get_duplicate_correlation(ne) > 0)
			printf("%u%% ", rtnl_netem_get_duplicate_correlation(ne));
	}
	
	printf("\n");
	
	return 0;
}
