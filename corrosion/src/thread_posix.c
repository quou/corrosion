#include <pthread.h>
#include <semaphore.h>

#include "thread.h"
#include "core.h"

void* _thread_worker(void* ptr) {
	((struct thread*)ptr)->working = true;

	((struct thread*)ptr)->worker(ptr);

	((struct thread*)ptr)->working = false;

	return null;
}

void init_thread(struct thread* thread, thread_worker_t worker) {
	thread->worker = worker;
}

void deinit_thread(struct thread* thread) {
	thread_join(thread);
}

void thread_execute(struct thread* thread) {
	if (thread->working) {
		warning("Thread already active.");
		return;
	}

	pthread_create(&thread->handle, null, _thread_worker, thread);
}

void thread_join(struct thread* thread) {
	if (!thread->handle) { return; }

	pthread_join(thread->handle, null);
	thread->handle = 0;
}

bool thread_active(const struct thread* thread) {
	return thread->working;
}

void* get_thread_uptr(const struct thread* thread) {
	return thread->uptr;
}

void set_thread_uptr(struct thread* thread, void* ptr) {
	thread->uptr = ptr;
}

void init_mutex(struct mutex* mutex) {
	pthread_mutex_init(&mutex->mutex, null);
}

void deinit_mutex(struct mutex* mutex) {
	pthread_mutex_destroy(&mutex->mutex);
}

void lock_mutex(struct mutex* mutex) {
	pthread_mutex_lock(&mutex->mutex);
}

void unlock_mutex(struct mutex* mutex) {
	pthread_mutex_unlock(&mutex->mutex);
}
