#include <libnl3/netlink/route/qdisc/netem.h>
#include <libnl3/netlink/route/qdisc/prio.h>
#include <libnl3/netlink/route/cls/fw.h>

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

int tc_init_prio(struct nl_sock *sock)
{
	q = rtnl_qdisc_alloc();

	rtnl_tc_set_ifindex(TC_CAST(q), if_index);
	rtnl_tc_set_parent(TC_CAST(q), TC_H_ROOT);
	rtnl_tc_set_handle(TC_CAST(q), TC_HANDLE(1, 0));
	rtnl_tc_set_kind(TC_CAST(q), "prio"); 

	rtnl_qdisc_prio_set_bands (struct rtnl_qdisc *qdisc, int bands);
	rtnl_qdisc_prio_set_priomap (struct rtnl_qdisc *qdisc, uint8_t priomap[], int len);

	rtnl_qdisc_add(sock, q, NLM_F_CREATE);
	rtnl_qdisc_put(q);
}

int tc_init_netem(struct nl_sock *sock, struct tc_netem *ne)
{
	q = rtnl_qdisc_alloc();

	rtnl_tc_set_ifindex(TC_CAST(q), if_index);
	rtnl_tc_set_parent(TC_CAST(q), TC_H_ROOT);
	rtnl_tc_set_handle(TC_CAST(q), TC_HANDLE(1, 0));
	rtnl_tc_set_kind(TC_CAST(q), "netem"); 

	rtnl_netem_set_gap(q, int);
	rtnl_netem_set_reorder_probability(q, int);
	rtnl_netem_set_reorder_correlation(q, int);
	rtnl_netem_set_corruption_probability(q, int);
	rtnl_netem_set_corruption_correlation(q, int);
	rtnl_netem_set_loss(q, int);
	rtnl_netem_set_loss_correlation(q, int);
	rtnl_netem_set_duplicate(q, int);
	rtnl_netem_set_duplicate_correlation(q, int);
	rtnl_netem_set_delay(q, int);
	rtnl_netem_set_jitter(q, int);
	rtnl_netem_set_delay_correlation(q, int);
	rtnl_netem_set_delay_distribution(q, const char *);

	rtnl_qdisc_add(sock, q, NLM_F_CREATE);
	rtnl_qdisc_put(q);
}

int tc_init_classifier(struct nl_sock *sock, int mark)
{
	struct rtnl_cls *c = rtnl_cls_alloc();

	rtnl_tc_set_link(TC_CAST(c), if_index);
	rtnl_tc_set_parent(TC_CAST(c), TC_H_ROOT);
	rtnl_tc_set_handle(TC_CAST(c), TC_HANDLE(1, 0));
	rtnl_tc_set_kind(TC_CAST(c), "netem"); 

	rtnl_fw_set_classid(c, uint32_t classid);
	rtnl_fw_set_mask(c, uint32_t mask);

	rtnl_cls_add(sock, cls, int flags);
	rtnl_cls_put(cls);
}

struct nl_sock * tc_init(int mark)
{
	struct nl_sock *sock;
	struct rtnl_link *link;
	struct rtnl_qdisc *q;

	/* Create connection to netlink */
	sock = nl_socket_alloc();
	nl_connect(sock, NETLINK_ROUTE);

	if (tc_init_prio(sock))
		error(0, -1, "Failed to setup TC: prio qdisc");
	if (tc_init_classifier(sock, mark))
		error(0, -1, "Failed to setup TC: fw filter");
	
	/* Get interface index */
	int ifindex = tc_get_ifindex;

	/* Setup classifier */
	rtnl_fw_set_classid (struct rtnl_cls *cls, uint32_t classid);
	rtnl_fw_set_mask (struct rtnl_cls *cls, uint32_t mask);
	rtnl_cls_add(struct nl_sock *sk, struct rtnl_cls *cls, int flags);

	return sock;
}

int tc_reset(struct nl_sock *)
{
	struct rtnl_qdisc *q;

	/* Restore default qdisc by deleting the root qdisc (see tc-pfifo_fast(8)) */
	q = rtnl_qdisc_alloc();
	rtnl_tc_set_link(TC_CAST(q), link);
	rtnl_qdisc_delete(sock, q); 
	rtnl_qdisc_put(q);

	/* Shutdown */
	nl_close(sock);
	nl_socket_free(sock);
}

