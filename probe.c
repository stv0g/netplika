#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <error.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "ts.h"

int probe()
{
	char *dev = "lo";

	printf("Start probing\n");
	
	/* Create RAW socket */
	int sd = socket(AF_INET, SOCK_RAW, 88);
	if (sd < 0)
		error(-1, errno, "Failed to create socket");

	if (ts_enable_if(dev))
		error(-1, errno, "Failed to enable timestamping for interface: %s", dev);
	if (ts_enable_sd(sd))
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
	
	if (sendto(sd, "test", sizeof("test"), 0, (struct sockaddr*) &sa, sizeof(sa)) == -1)
		error(-1, errno, "Failed to send");

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
	if (ts_recvmsg(sd, &msgh, (dir == TX) ? MSG_ERRQUEUE : 0, ts, &key, &id) < 0)
		error(-1, errno, "Failed to receive TS");
	
	printf("Received key=%d, id=%d\n", key, id);
	
	for (int i = 0; i < 3; i++)
		ts_print(&ts[i]);

	return 0;
}
