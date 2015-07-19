/** Probing for RTT, Loss, Duplication, Corruption.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

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

#include <netlink/addr.h>

#include <linux/ip.h>

#include "ts.h"
#include "timing.h"
#include "config.h"
#include "tcp.h"
#include "utils.h"
#include "hist.h"

int probe_tcp(int sd, struct timespec *ts)
{
	struct timespec ts_syn, ts_ack;
	
	struct iphdr  ihdr;
	struct tcphdr thdr;
	struct msghdr msgh;
	ssize_t len;

	struct iovec iov[2] = {
		{ .iov_base = &ihdr, .iov_len = sizeof(ihdr) }, // IP header
		{ .iov_base = &thdr, .iov_len = sizeof(thdr) }  // TCP header
	};

	/* Sending SYN */
	memset(&ihdr, 0, sizeof(ihdr));
	memset(&thdr, 0, sizeof(thdr));
	memset(&msgh, 0, sizeof(msgh));
	
	unsigned seq = (unsigned) rand();
	
	thdr.syn = 1;
	thdr.seq = htonl(seq);
	thdr.source = htons(SRC_PORT); 
	thdr.dest = htons(DEST_PORT);
	thdr.doff = 5;
	thdr.check = tcp_csum((unsigned short *) &thdr, sizeof(thdr));
	
	msgh.msg_iov = &iov[1]; // only send TCP header
	msgh.msg_iovlen = 1;
	
	if (ts_sendmsg(sd, &msgh, 0, &ts_syn) < 0)
		error(-1, errno, "Failed to send SYN packet");
		
	/* Receiving ACK */
	memset(&ihdr, 0, sizeof(ihdr));
	memset(&thdr, 0, sizeof(thdr));

	msgh.msg_iov = &iov[0]; // receive IP + TCP header
	msgh.msg_iovlen = 2;
	
retry:	len = ts_recvmsg(sd, &msgh, 0, &ts_ack);
	if (len < 0)
		error(-1, 0, "Failed to receive ACK / RST packet");

	//printf("TCP: len=%u, syn=%u, ack=%u, rst=%u, seq=%u, ack_seq=%u, src=%u, dst=%u\n",
	//	len, thdr.syn, thdr.ack, thdr.rst, ntohl(thdr.seq), ntohl(thdr.ack_seq), ntohs(thdr.source), ntohs(thdr.dest));
	
	/* Check response */
	if (thdr.source != htons(DEST_PORT) || thdr.dest != htons(SRC_PORT)) {
		printf("Skipping invalid ports\n");
		goto retry;
	}
	else if (!thdr.rst && !(thdr.ack && thdr.syn)) {
		printf("Skipping invalid flags\n");
		goto retry;
	}
	else if (ntohl(thdr.ack_seq) != seq + 1) {
		printf("Skipping invalid seq\n");
		goto retry;
	}
	
	*ts = time_diff(&ts_syn, &ts_ack);
	
	return 0;
}

int probe(int argc, char *argv[])
{
	/* Parse address */
	struct nl_addr *addr;
	struct sockaddr_in sin;
	char buf[128];
	
	if (argc != 1)
		error(-1, 0, "usage: netem probe IP-ADDRESS");
	
	if (nl_addr_parse(argv[0], AF_UNSPEC, &addr))
		error(-1, 0, "Failed to parse address: %s", argv[0]);
	
	printf("Start probing: %s\n", nl_addr2str(addr, buf, sizeof(buf)));
	
	socklen_t sinlen = sizeof(sin);
	if (nl_addr_fill_sockaddr(addr, (struct sockaddr *) &sin, &sinlen))
		error(-1, 0, "Failed to fill sockaddr");
	
	/* Create RAW socket */
	int sd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sd < 0)
		error(-1, errno, "Failed to create socket");
	
	/* Bind socket to destination */
	if (connect(sd, (struct sockaddr *) &sin, sizeof(sin)))
		error(-1, 0, "Failed to connect socket");

	/* Enable Kernel TS support */
	if (ts_enable_if("lo"))
		perror("Failed to enable timestamping");
	if (ts_enable_sd(sd))
		perror("Failed to set SO_TIMESTAMPING");
	
	/* Prepare payload */
	struct timespec ts;
	
	/* Prepare statistics */
	struct hist hist;
	hist_create(&hist, 0, 0, 1);
	
	while (hist.total < cfg.limit) {		
		probe_tcp(sd, &ts);

		double rtt = time_to_double(&ts);
		hist_put(&hist, rtt);
		
		printf("n=%u, rtt=%f, min=%f, max=%f, avg=%f, stddev=%f\n",
			hist.total, rtt, hist.lowest, hist.highest, hist_mean(&hist), hist_stddev(&hist));
		
		/* Warmup: adjust histogram after rough estimation of RTT */
		if (hist.total == 20 && hist.high == 0) {
			double span = hist.highest - hist.lowest;
			hist_destroy(&hist);
			hist_create(&hist, MAX(0, hist.lowest - span * 0.1), hist.highest + span * 0.2, 1e-3);
			printf("Created new histogram: high=%f, low=%f, buckets=%u\n",
				hist.high, hist.low, hist.length);
		}
		
		//usleep(250 * 1e3); // 250 ms
	}
	
	hist_print(&hist);
	hist_destroy(&hist);

	return 0;
}