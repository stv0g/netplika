/** Main routine.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2015, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <error.h>

#include "config.h"

int running = 1;

/* Default settings */
struct config cfg = {
	.limit = 100,
	.mark = 0xCD,
	.mask = 0xFFFFFFFF,
	.dev = "eth0"
};

int probe(int argc, char *argv[]);
int emulate(int argc, char *argv[]);

void quit(int sig, siginfo_t *si, void *ptr)
{
	printf("Goodbye!\n");
	exit(0);
}

int main(int argc, char *argv[])
{	
	if (argc < 2) {
		printf("usage: %s CMD [OPTIONS]\n", argv[0]);
		printf("  CMD     can be one of:\n");
		printf("    live          \n");
		printf("    probe         \n");
		printf("    emulate       \n");		
		printf("\n");
		printf("  OPTIONS:\n");
		printf("    -m --mark N      apply emulation only to packet buffers with mark N\n");
		printf("    -M --mask N      an optional mask for the fw mark\n");
		printf("    -i --interval N  update the emulation parameters every N seconds\n");
		printf("    -r --rate        the packet rate used for measurements\n");
		printf("    -l --limit       how many probes should we sent\n");
		printf("    -d --dev         network interface\n");
		
		printf("\n");
		printf("netem util %s (built on %s %s)\n",
			VERSION, __DATE__, __TIME__);
		printf(" Copyright 2015, Steffen Vogel <post@steffenvogel.de>\n");

		exit(EXIT_FAILURE);
	}
	
	char *cmd = argv[1];

	/* Setup signals */
	struct sigaction sa_quit = {
		.sa_flags = SA_SIGINFO,
		.sa_sigaction = quit
	};

	sigemptyset(&sa_quit.sa_mask);
	sigaction(SIGTERM, &sa_quit, NULL);
	sigaction(SIGINT, &sa_quit, NULL);
	
	/* Initialize PRNG for TCP sequence nos */
	srand(time(NULL));
	
	/* Parse Arguments */
	char c, *endptr;
	while ((c = getopt (argc-1, argv+1, "h:m:M:i:l:d:")) != -1) {
		switch (c) {
			case 'm':
				cfg.mark = strtoul(optarg, &endptr, 0);
				goto check;
			case 'M':
				cfg.mask = strtoul(optarg, &endptr, 0);
				goto check;
			case 'i':
				cfg.interval = strtoul(optarg, &endptr, 10);
				goto check;
			case 'r':
				cfg.rate = strtoul(optarg, &endptr, 10);
				goto check;
			case 'l':
				cfg.limit = strtoul(optarg, &endptr, 10);
				goto check;
				
			case 'd':
				cfg.dev = strdup(optarg);
				break;
				
			case '?':
				if (optopt == 'c')
					error(-1, 0, "Option -%c requires an argument.", optopt);
				else if (isprint(optopt))
					error(-1, 0, "Unknown option '-%c'.", optopt);
				else
					error(-1, 0, "Unknown option character '\\x%x'.", optopt);
				exit(EXIT_FAILURE);
			default:
				abort();
		}
		
		continue;
check:
		if (optarg == endptr)
			error(-1, 0, "Failed to parse parse option argument '-%c %s'", c, optarg);
	}

	if (!strcmp(cmd, "probe"))
		return probe(argc-optind-1, argv+optind+1);
	else if (!strcmp(cmd, "emulate"))
		return emulate(argc-optind-1, argv+optind+1);
	else
		error(-1, 0, "Unknown command: %s", cmd);
	
	return 0;
}