#pragma once

#include "common.h"

struct thread;

typedef void (*thread_worker_t)(struct thread* thread);

#if defined(_WIN32)
#error Not supported
#else
#include <pthread.h>

struct thread {
	pthread_t handle;
	void* uptr;
	bool working;
	thread_worker_t worker;
};

struct mutex {
	pthread_mutex_t mutex;
};

#endif

void init_thread(struct thread* thread, thread_worker_t worker);
void deinit_thread(struct thread* thread);
void thread_execute(struct thread* thread);
void thread_join(struct thread* thread);
bool thread_active(const struct thread* thread);
void* get_thread_uptr(const struct thread* thread);
void  set_thread_uptr(struct thread* thread, void* ptr);

void init_mutex(struct mutex* mutex);
void deinit_mutex(struct mutex* mutex);
void lock_mutex(struct mutex* mutex);
void unlock_mutex(struct mutex* mutex);
