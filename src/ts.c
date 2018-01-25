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

ssize_t ts_sendmsg(int sd, const struct msghdr *msg, int flags, struct timespec *ts)
{
	ssize_t ret = sendmsg(sd, msg, flags);

	/* Wait for TS */
	struct pollfd pfd = {
		.fd = sd,
		.events = POLLIN
	};

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

	struct scm_timestamping {
		struct timespec ts[3];
	} *scmts;

	ssize_t ret = recvmsg(sd, msgh, flags);
	if (ret >= 0) {
		for (cmsg = CMSG_FIRSTHDR(msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(msgh, cmsg)) {
			if       (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING)
				scmts = (struct scm_timestamping *) CMSG_DATA(cmsg);
			else if ((cmsg->cmsg_level == SOL_IP   && cmsg->cmsg_type == IP_RECVERR) ||
				 (cmsg->cmsg_level == SOL_IPV6 && cmsg->cmsg_type == IPV6_RECVERR))
				serr = (struct sock_extended_err *) CMSG_DATA(cmsg);
		}

		if (scmts) {
			*ts = scmts->ts[0];
		}
	}

	return ret;
}

int ts_enable(int sd)
{
	int val;

	/* Enable kernel / hw timestamping */
	val  = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE;	/* Enable SW timestamps */
	val |= SOF_TIMESTAMPING_SOFTWARE; 					/* Report SW and HW timestamps */
	val |= SOF_TIMESTAMPING_OPT_TSONLY;					/* Only return timestamp in cmsg */
	val |= SOF_TIMESTAMPING_OPT_ID;

	return setsockopt(sd, SOL_SOCKET, SO_TIMESTAMPING, (void *) &val, sizeof(val));
}
