/** Probing for RTT, Loss, Duplication, Corruption.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

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
#include <sys/timerfd.h>

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <netlink/addr.h>

#include <linux/ip.h>
#include <linux/icmp.h>

#include "ts.h"
#include "timing.h"
#include "config.h"
#include "utils.h"
#include "hist.h"

struct phdr {
	uint32_t source;
	uint32_t destination;
	uint8_t reserved;
	uint8_t protocol;
	uint16_t length;
} __attribute__((packed));

struct icmppl { // ICMP payload
	uint64_t counter;
} __attribute__((packed));

struct timespec past_ts_req[1024];
static uint64_t counter_tx = 0;
static uint64_t counter_rx = 0;

int probe_icmp_tx(int sd, struct sockaddr_in *dst)
{
	struct timespec ts_req;
	ssize_t bytes, wbytes;

	bytes = sizeof(struct icmphdr) + sizeof(struct icmppl) + cfg.probe.payload;

	struct msghdr msgh;
	struct iovec iov;
	char buf[bytes];

	memset(buf, 0, sizeof(buf));
	memset(&msgh, 0, sizeof(msgh));
	memset(&iov, 0, sizeof(iov));

	struct icmphdr *ichdr = (struct icmphdr *) buf;
	struct icmppl *icpl = (struct icmppl *) (ichdr + 1);

	ichdr->type = ICMP_ECHO;
	ichdr->code = 0;
	ichdr->checksum = 0;
	ichdr->un.echo.id = 1;
	ichdr->un.echo.sequence = rand();

	icpl->counter = counter_tx++;

	ichdr->checksum = chksum_rfc1071((char *) ichdr, bytes);

	iov.iov_base = ichdr;
	iov.iov_len = bytes;

	msgh.msg_name = dst;
	msgh.msg_namelen = sizeof(struct sockaddr_in);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;

	wbytes = ts_sendmsg(sd, &msgh, 0, &ts_req);
	if (bytes < 0)
		error(-1, errno, "Failed to send ICMP echo request");

	if (wbytes != bytes)
		fprintf(stderr, "wbytes(%zd) != bytes(%zd)\n", wbytes, bytes);

	past_ts_req[icpl->counter % 1024] = ts_req;

	return 0;
}

int probe_icmp_rx(int sd)
{
	struct timespec ts_rep;
	ssize_t bytes, rbytes;

	bytes = sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct icmppl);

	struct msghdr msgh;
	struct iovec iov;
	char buf[bytes];

	memset(buf, 0, sizeof(buf));
	memset(&msgh, 0, sizeof(msgh));
	memset(&iov, 0, sizeof(iov));

	struct iphdr *ihdr = (struct iphdr *) buf;
	struct icmphdr *ichdr = (struct icmphdr *) (ihdr + 1);
	struct icmppl *icpl = (struct icmppl *) (ichdr + 1);

	iov.iov_base = ihdr;
	iov.iov_len = bytes;

	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;

	rbytes = ts_recvmsg(sd, &msgh, MSG_DONTWAIT, &ts_rep);
	if (rbytes < 0) {
		if (errno == EAGAIN)
			return 1;
		else
			error(-1, errno, "Failed to receive ICMP echo reply");
	}

	if (rbytes != bytes)
		fprintf(stderr, "rbytes(%zd) != bytes(%zd)\n", rbytes, bytes);

	if (counter_tx - icpl->counter < 1024)
		printf("%zd,%zd,%.10e\n", counter_rx, icpl->counter, time_delta(&past_ts_req[icpl->counter % 1024], &ts_rep));

	counter_rx++;

	return 0;
}

int probe_tcp(int sd, struct sockaddr_in *src, struct sockaddr_in *dst, struct timespec *ts)
{
	struct timespec ts_syn, ts_ack;
	ssize_t bytes, rbytes, wbytes;
	struct phdr   *phdr;
	struct tcphdr *thdr;

	bytes = sizeof(struct phdr) + sizeof(struct tcphdr);
	char buf[bytes];

	struct msghdr msgh;
	struct iovec iov;

	memset(buf, 0, sizeof(buf));
	memset(&iov, 0, sizeof(iov));
	memset(&msgh, 0, sizeof(msgh));

	/* Randomize sequence number and source port */
	unsigned int seq = (unsigned) rand();
	unsigned short sport = (rand() + 1024) & 0xFFFF;

	phdr = (struct phdr *) buf;
	thdr = (struct tcphdr *) (phdr + 1);

	phdr->source = src->sin_addr.s_addr;
	phdr->destination = dst->sin_addr.s_addr;
	phdr->protocol = IPPROTO_TCP;
	phdr->length = htons(sizeof(struct tcphdr));

	thdr->source = htons(sport);
	thdr->dest = dst->sin_port;
	thdr->syn = 1;
	thdr->ack = 0;
	thdr->seq = htonl(seq);
	thdr->doff = 5;
	thdr->check = 0;

	thdr->check = chksum_rfc1071((char *) phdr, bytes);

	hexdump(phdr, bytes);

	iov.iov_base = thdr;
	iov.iov_len = sizeof(struct tcphdr);

	msgh.msg_name = dst;
	msgh.msg_namelen = sizeof(struct sockaddr_in);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;

	wbytes = ts_sendmsg(sd, &msgh, 0, &ts_syn);
	if (wbytes < 0)
		error(-1, errno, "Failed to send SYN packet");

	msgh.msg_name = NULL;

retry:	rbytes = ts_recvmsg(sd, &msgh, 0, &ts_ack);
	if (rbytes < 0)
		error(-1, 0, "Failed to receive ACK / RST packet");

	printf("TCP: rbytes=%zd, syn=%u, ack=%u, rst=%u, seq=%u, ack_seq=%u, src=%u, dst=%u\n",
		rbytes, thdr->syn, thdr->ack, thdr->rst, ntohl(thdr->seq), ntohl(thdr->ack_seq), ntohs(thdr->source), ntohs(thdr->dest));

	/* Check response */
	if (thdr->source != dst->sin_port || thdr->dest != sport) {
//		printf("Skipping invalid ports\n");
//		goto retry;
	}
	else if (!thdr->rst && !(thdr->ack && thdr->syn)) {
		printf("Skipping invalid flags\n");
		goto retry;
	}
	else if (ntohl(thdr->ack_seq) != seq + 1) {
		printf("Skipping invalid seq\n");
		goto retry;
	}

	*ts = time_diff(&ts_syn, &ts_ack);

	return 0;
}

int probe(int argc, char *argv[])
{
	int run = 0, tfd, sd, ret, prot;
	socklen_t sinlen;

	if (cfg.probe.mode == PROBE_TCP) {
		prot = IPPROTO_TCP;
	}
	else if (cfg.probe.mode == PROBE_ICMP) {
		prot = IPPROTO_ICMP;
	}
	else {
		error(-1, 0, "Invalid probe mode");
	}

	/* Parse address */
	struct nl_addr *addr;
	struct sockaddr_in src, dst;

	/* Parse args */
	if (argc != 2)
		error(-1, 0, "usage: netem probe IP [PORT]");

	/* Destination */
	ret = nl_addr_parse(argv[0], AF_INET, &addr);
	if (ret)
		error(-1, 0, "Failed to parse address: %s", argv[0]);

	sinlen = sizeof(src);
	if (nl_addr_fill_sockaddr(addr, (struct sockaddr *) &dst, &sinlen))
		error(-1, 0, "Failed to fill sockaddr");

	/* Source */
	ret = nl_addr_parse("10.211.55.6", AF_INET, &addr);
	if (ret)
		error(-1, 0, "Failed to parse address: %s", argv[0]);

	sinlen = sizeof(dst);
	if (nl_addr_fill_sockaddr(addr, (struct sockaddr *) &src, &sinlen))
		error(-1, 0, "Failed to fill sockaddr");

	int port = atoi(argv[1]);
	if (!port)
		error(-1, 0, "Failed to parse port: %s", argv[1]);

	dst.sin_port = htons(port);

	/* Create RAW socket */
	sd = socket(AF_INET, SOCK_RAW, prot);
	if (sd < 0)
		error(-1, errno, "Failed to create socket");

	/* Enable Kernel TS support */
	ret = ts_enable(sd);
	if (ret)
		fprintf(stderr, "Failed to set SO_TIMESTAMPING: %s\n", strerror(errno));

	/* Prepare payload */
	struct timespec ts;

	/* Prepare statistics */

	/* Start timer */
	if ((tfd = timerfd_init(cfg.probe.rate)) < 0)
		error(-1, errno, "Failed to initilize timer");

	if (cfg.probe.mode == PROBE_ICMP) {
		do {
			if (!cfg.probe.limit || run < cfg.probe.limit)
				probe_icmp_tx(sd, &dst);

			do {
				ret = probe_icmp_rx(sd);
			} while (!ret);

			run += timerfd_wait(tfd);
		} while (cfg.probe.limit && run < 2*cfg.probe.limit);
	}
	else if (cfg.probe.mode == PROBE_TCP) {
		do {
			probe_tcp(sd, &src, &dst, &ts);

			double rtt = time_to_double(&ts);

			printf("%d,%.10f\n", run, rtt);

			run += timerfd_wait(tfd);
		} while (cfg.probe.limit && run < cfg.probe.limit);
	}

	return 0;
}
