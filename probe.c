#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

int probe()
{
	int val;
	
	printf("Start probing\n");
	
	/* Create RAW socket */
	int sd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

	/* Enable kernel / hw timestamping */
	val  = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RX_SOFTWARE;	/* Enable RX timestamps */
	val |= SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_TX_SOFTWARE;	/* Enable TX timestamps */
	
	val |= SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;	/* Report SW and HW timestamps */
	
	val |= SOF_TIMESTAMPING_OPT_TSONLY;					/* Only return timestamp in cmsg */

	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, (void *) val, &val))
		error(-1, errno, "Failed to set SO_TIMESTAMPING");
	

	return 0;
}

int probe_read_ts(int sd)
{
	char cmsg[1024];
	struct msghdr msgh = {
		.msg_name    = NULL,  .msg_namelen    = 0,
		.msg_iov     = NULL,  .msg_iovlen     = 0,
		.msg_control = &cmsg, .msg_controllen = sizeof(cmsg)
	};
	struct cmsghdr *cmsg;
	struct timespec *ts;

	if (recvmsg(sd, &msgh, 0) == -1)
		error(-1, errno, "Failed to get control messages");

	/* Receive auxiliary data in msgh */
	for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh,cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING) {
			ts = (struct timespec*) CMSG_DATA(cmsg);
			break;
		}
	}
	if (cmsg == NULL)
		error(-1, 0, "SO_TIMESTAMPING not enabled or small buffer or I/O error.");
	
	if (ts) {
		for (int i = 0; i < 3; i++) {
			print_ts(&ts[i]);
		}
	}
}