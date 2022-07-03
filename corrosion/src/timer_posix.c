#include <time.h>
#include <sys/time.h>

#include "timer.h"

static clockid_t global_clock;
static u64 global_freq;

u64 init_timer() {
	global_clock = CLOCK_REALTIME;
	global_freq = 1000000000;

#if defined(_POSIX_MONOTONIC_CLOCK)
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		global_clock = CLOCK_MONOTONIC;
	}
#endif
}

u64 get_timer() {
	struct timespec ts;

	clock_gettime(global_clock, &ts);
	return (u64)ts.tv_sec * global_freq + (u64)ts.tv_nsec;
}

u64 get_timer_frequency() {
	return global_freq;
}
