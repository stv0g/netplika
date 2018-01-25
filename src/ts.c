/** Hardware / Kernelspace Timestamping of network packets.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <error.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <net/if.h>

#include <linux/if.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>

#include "ts.h"

#define SIOCSHWTSTAMP	0x89b0
#define SIOCGHWTSTAMP	0x89b1

ssize_t ts_sendmsg(int sd, const struct msghdr *msg, int flags, struct timespec *ts)
{
	/* Waiting for ACK or RST */
	struct pollfd pfd = {
		.fd = sd,
		.events = POLLIN
	};
	
	clock_gettime(CLOCK_REALTIME, ts); // Fallback if now kernelspace TS are available
	ssize_t ret = sendmsg(sd, msg, flags);
	return ret; // TODO
	
	/* Wait for TS */
retry:	if (poll(&pfd, 1, 10000) < 0)
		error(-1, errno, "Failed poll");
	if (!(pfd.revents & POLLERR))
		goto retry;

	struct msghdr msg2 = { 0 };
	if (ts_recvmsg(sd, &msg2, MSG_ERRQUEUE, ts) < 0)
		error(-1, errno, "Failed to receive TS");
	
	return ret;
}

ssize_t ts_recvmsg(int sd, struct msghdr *msgh, int flags, struct timespec *ts)
{
	char buf[1024];
	msgh->msg_control = &buf;
	msgh->msg_controllen = sizeof(buf);

	struct cmsghdr *cmsg;
	struct sock_extended_err *serr = NULL;
	struct timespec *tss = NULL;

	ssize_t ret = recvmsg(sd, msgh, flags);
	if (ret >= 0) {
		clock_gettime(CLOCK_REALTIME, ts); // Fallback if now kernelspace TS are available
		return ret; // TODO
		
		for (cmsg = CMSG_FIRSTHDR(msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(msgh, cmsg)) {
			if       (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING)
				tss = (struct timespec *) CMSG_DATA(cmsg);
			else if ((cmsg->cmsg_level == SOL_IP	 && cmsg->cmsg_type == IP_RECVERR) ||
				 (cmsg->cmsg_level == SOL_IPV6 && cmsg->cmsg_type == IPV6_RECVERR))
				serr = (struct sock_extended_err *) CMSG_DATA(cmsg);
		}
	
		if (tss)
			*ts = tss[0];
	
		if (serr)
			printf("Debug: key = %u, id = %u\n", serr->ee_info, serr->ee_data);
	}

	return ret;
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

