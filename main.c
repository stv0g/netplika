#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <error.h>

#include <signal.h>
#include <unistd.h>

#define VERSION "0.1"

int running = 1;

void quit(int sig, siginfo_t *si, void *ptr)
{
	printf("Goodbye!\n");
	running = 0;
}

int main(int argc, char *argv[])
{
	/* Options */
	int mark;
	int interval;
	int rate;
	
	if (argc < 2) {
		printf("usage: %s CMD [OPTIONS]\n", argv[0]);
		printf("  CMD     can be one of:\n");
		printf("    live          \n");
		printf("    probe         \n");
		printf("    emulate       \n");		
		
		/*
		  -m --mark N                 apply emulation only to packet buffers with mark N
		  -i --interval N             update the emulation parameters every N seconds
		  -r --rate                   the packet rate used for measurements
		*/
		
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
	
	/* Parse Arguments */
	char c, *endptr;
	while ((c = getopt (argc-1, argv+1, "h:m:i:")) != -1) {
		switch (c) {
			case 'm':
				mark = strtoul(optarg, &endptr, 10);
				goto check;
			case 'i':
				interval = strtoul(optarg, &endptr, 10);
				goto check;
			case 'r':
				rate = strtoul(optarg, &endptr, 10);
				goto check;
				
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
		return probe();
	else
		error(-1, 0, "Unknown command: %s", cmd);
	
	return 0;
}