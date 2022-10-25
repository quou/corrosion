#include "thread.h"
#include "core.h"

void init_thread(struct thread* thread, thread_worker_t worker) {
	abort_with("Not implemented.");
}

void deinit_thread(struct thread* thread) {
	abort_with("Not implemented.");
}

void thread_execute(struct thread* thread) {
	abort_with("Not implemented.");
}

void thread_join(struct thread* thread) {
	abort_with("Not implemented.");
}

bool thread_active(const struct thread* thread) {
	abort_with("Not implemented.");
}

void* get_thread_uptr(const struct thread* thread) {
	abort_with("Not implemented.");
}

void set_thread_uptr(struct thread* thread, void* ptr) {
	abort_with("Not implemented.");
}

void init_mutex(struct mutex* mutex) {
	abort_with("Not implemented.");
}

void deinit_mutex(struct mutex* mutex) {
	abort_with("Not implemented.");
}

void lock_mutex(struct mutex* mutex) {
	abort_with("Not implemented.");
}

void unlock_mutex(struct mutex* mutex) {
	abort_with("Not implemented.");
}
