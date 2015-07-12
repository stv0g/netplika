#include <libnl3/netlink/route/tc.h>
#include <libnl3/netlink/route/qdisc.h>
#include <libnl3/netlink/route/classifier.h>

struct tc_netem {
	int gap;
	int reorder_prop;
	int reorder_corr;
	int corruption_prop;
	int corruption_corr;
	int loss_prop;
	int loss_corr;
	int duplication_prob;
	int duplication_corr;
	int jitter;
	int delay;
	int delay_corr;
	char *delay_distr;
};

struct rtnl_link * tc_get_link(struct nl_sock *sock, const char *dev);

int tc_init_prio(struct nl_sock *sock);

int tc_init_netem(struct nl_sock *sock, struct tc_netem *ne);

int tc_init_classifier(struct nl_sock *sock, int mark);

struct nl_sock * tc_init(int mark);

int tc_reset(struct nl_sock *);
