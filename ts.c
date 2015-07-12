#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <net/if.h>

#include <linux/if.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>

#define SIOCSHWTSTAMP   0x89b0
#define SIOCGHWTSTAMP   0x89b1

ssize_t ts_sendmsg(int sd, const struct msghdr *msg, int flags, struct timespec *ts)
{
	return sendmsg(sd, msg, flags);
}

ssize_t ts_recvmsg(int sd, struct msghdr *msgh, int flags, struct timespec *ts, int *key, int *id)
{
	char buf[1024];
	msgh->msg_control = &buf;
	msgh->msg_controllen = sizeof(buf);

	struct cmsghdr *cmsg;
	struct sock_extended_err *serr = NULL;
	struct timespec *tss = NULL;

	int ret = recvmsg(sd, msgh, flags);
	if (ret >= 0) {
		for (cmsg = CMSG_FIRSTHDR(msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(msgh, cmsg)) {
			if       (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING)
				tss = (struct timespec *) CMSG_DATA(cmsg);
			else if ((cmsg->cmsg_level == SOL_IP	 && cmsg->cmsg_type == IP_RECVERR) ||
				 (cmsg->cmsg_level == SOL_IPV6 && cmsg->cmsg_type == IPV6_RECVERR))
				serr = (struct sock_extended_err *) CMSG_DATA(cmsg);
		}
	
		if (!tss)
			return -2;
	
		for (int i = 0; i < 3; i++)
			ts[i] = tss[i];
	
		if (serr) {
			*key = serr->ee_info;
			*id  = serr->ee_data;
		}
	}

	return ret;
}

void ts_print(struct timespec *ts)
{
	printf("%lld.%.9ld\n", (long long) ts->tv_sec, ts->tv_nsec);
}

int ts_enable_sd(int sd)
{
	int val;

	/* Enable kernel / hw timestamping */
	val  = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE;	/* Enable SW timestamps */
//	val |= SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE;	/* Enable HW timestamps */
	val |= SOF_TIMESTAMPING_SOFTWARE; //| SOF_TIMESTAMPING_RAW_HARDWARE;	/* Report SW and HW timestamps */
//	val |= SOF_TIMESTAMPING_OPT_TSONLY;					/* Only return timestamp in cmsg */

	return setsockopt(sd, SOL_SOCKET, SO_TIMESTAMPING, (void *) &val, sizeof(val));
}

int ts_enable_if(const char *dev)
{
	int ret = -1, sd;

	struct hwtstamp_config tsc = {
		.flags = 0,
		.tx_type = HWTSTAMP_TX_ON,
		.rx_filter = HWTSTAMP_FILTER_ALL
	};
	struct ifreq ifr = {
		.ifr_data = (void *) &tsc
	};
	
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd >= 0)
		ret = ioctl(sd, SIOCSHWTSTAMP, &ifr);
	
	close(sd);
		
	return ret;
}

