/** Time related functions.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel 
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 199309L

#include <unistd.h>
	
#include <time.h>
#include <sys/timerfd.h>

#include "timing.h"

int timerfd_init(double rate)
{
	struct itimerspec its = {
		.it_interval = time_from_double(1 / rate),
		.it_value = { 1, 0 }
	};

	int tfd = timerfd_create(CLOCK_REALTIME, 0);
	if (tfd < 0)
		return -1;

	if (timerfd_settime(tfd, 0, &its, NULL))
		return -1;
	
	return tfd;
}

uint64_t timerfd_wait(int fd)
{
	uint64_t runs;
	
	return read(fd, &runs, sizeof(runs)) < 0 ? 0 : runs;
}

uint64_t timerfd_wait_until(int fd, struct timespec *until)
{
	struct itimerspec its = {
		.it_value = *until
	};
		
	if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &its, NULL))
		return 0;
	else
		return timerfd_wait(fd);
}

struct timespec time_add(struct timespec *start, struct timespec *end)
{
	struct timespec sum = {
		.tv_sec  = end->tv_sec  + start->tv_sec,
		.tv_nsec = end->tv_nsec + start->tv_nsec
	};

	if (sum.tv_nsec > 1000000000) {
		sum.tv_sec  += 1;
		sum.tv_nsec -= 1000000000;
	}
	
	return sum;
}

struct timespec time_diff(struct timespec *start, struct timespec *end)
{
	struct timespec diff = {
		.tv_sec  = end->tv_sec  - start->tv_sec,
		.tv_nsec = end->tv_nsec - start->tv_nsec
	};

	if (diff.tv_nsec < 0) {
		diff.tv_sec  -= 1;
		diff.tv_nsec += 1000000000;
	}
	
	return diff;
}

struct timespec time_from_double(double secs)
{
	struct timespec ts;
	
	ts.tv_sec  = secs;
	ts.tv_nsec = 1.0e9 * (secs - ts.tv_sec);

	return ts;
}

double time_to_double(struct timespec *ts)
{
	return ts->tv_sec + ts->tv_nsec * 1e-9;
}

double time_delta(struct timespec *start, struct timespec *end)
{
	struct timespec diff = time_diff(start, end);
	
	return time_to_double(&diff);
}

int time_fscan(FILE *f, struct timespec *ts)
{
	return fscanf(f, "%lu.%lu", &ts->tv_sec, &ts->tv_nsec);
}

int time_fprint(FILE *f, struct timespec *ts)
{
	return fprintf(f, "%lu.%09lu\t", ts->tv_sec, ts->tv_nsec);
}