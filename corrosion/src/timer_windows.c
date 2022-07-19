#include <Windows.h>

#include "timer.h"

u64 global_freq;
bool global_has_pc;

void init_timer() {
	if (QueryPerformanceFrequency((LARGE_INTEGER*)&global_freq)) {
		global_has_pc = true;
	} else {
		global_freq = 1000;
		global_has_pc = false;
	}
}

u64 get_timer() {
	u64 now;

	if (global_has_pc) {
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
	} else {
		now = (u64)timeGetTime();
	}

	return now;
}

u64 get_timer_frequency() {
	return global_freq;
}
