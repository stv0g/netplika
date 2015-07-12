#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <error.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <net/if.h>

#include <linux/if.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>


#define SIOCSHWTSTAMP   0x89b0
#define SIOCGHWTSTAMP   0x89b1

void print_ts(struct timespec *ts)
{
	printf("%lld.%.9ld\n", (long long) ts->tv_sec, ts->tv_nsec);
}

int if_enable_ts(const char *dev)
{
	struct hwtstamp_config tsc = {
		.flags = 0,
		.tx_type = HWTSTAMP_TX_ON,
		.rx_filter = HWTSTAMP_FILTER_ALL
	};
	struct ifreq ifr = {
		.ifr_data = (void *) &tsc
	};
	
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		error(-1, errno, "Failed to create socket");

	if (ioctl(sd, SIOCSHWTSTAMP, &ifr))
		error(-1, errno, "Failed to enable timestamping");
	
	close(sd);
		
	return 0;
}

int recvmsg_ts(int sd, struct msghdr *msgh, int flags, struct timespec *ts, int *key, int *id)
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

int probe()
{
	int val;
	
	printf("Start probing\n");
	
	//if_enable_ts("lo");
	
	/* Create RAW socket */
	int sd = socket(AF_INET, SOCK_RAW, 88);
	if (sd < 0)
		error(-1, errno, "Failed to create socket");

	/* Enable kernel / hw timestamping */
	val  = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE;	/* Enable SW timestamps */
//	val |= SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE;	/* Enable HW timestamps */
	val |= SOF_TIMESTAMPING_SOFTWARE; //| SOF_TIMESTAMPING_RAW_HARDWARE;	/* Report SW and HW timestamps */
//	val |= SOF_TIMESTAMPING_OPT_TSONLY;					/* Only return timestamp in cmsg */

	if (setsockopt(sd, SOL_SOCKET, SO_TIMESTAMPING, (void *) &val, sizeof(val)))
		error(-1, errno, "Failed to set SO_TIMESTAMPING");
	
	struct timespec ts[3];
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(7),
	};
	struct msghdr msgh = {
		.msg_name = (struct sockaddr *) &sa,
		.msg_namelen = sizeof(sa)
	};
	
	inet_aton("8.8.8.8", &sa.sin_addr);
	
	//if (sendto(sd, "test", sizeof("test"), 0, (struct sockaddr*) &sa, sizeof(sa)) == -1)
	//	error(-1, errno, "Failed to send");

	enum direction { TX, RX } dir = RX;

	/* Polling for new timestamps */
	struct pollfd pfd = {
		.fd = sd,
		.events = POLLIN
	};
retry:	if (poll(&pfd, 1, 10000) < 0)
		error(-1, errno, "Failed poll");
	if ((dir == TX) && !(pfd.revents & POLLERR))
		goto retry;

	int id, key;
	if (recvmsg_ts(sd, &msgh, (dir == TX) ? MSG_ERRQUEUE : 0, ts, &key, &id) < 0)
		error(-1, errno, "Failed to receive TS");
	
	printf("Received key=%d, id=%d\n", key, id);
	
	for (int i = 0; i < 3; i++)
		print_ts(&ts[i]);

	return 0;
}
