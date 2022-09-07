#include <windows.h>

#include "thread.h"
#include "core.h"

DWORD WINAPI _thread_worker(LPVOID lparam) {
	struct thread* thread = (struct thread*)lparam;

	thread->working = true;

	thread->worker(thread);

	thread->working = false;

	return 0;
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

	thread->handle = CreateThread(null, 0, _thread_worker, thread, 0, &thread->id);
}

void thread_join(struct thread* thread) {
	if (!thread->handle) { return; }

	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
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
	mutex->handle = CreateMutex(null, false, null);
}

void deinit_mutex(struct mutex* mutex) {
	CloseHandle(mutex->handle);
}

void lock_mutex(struct mutex* mutex) {
	WaitForSingleObject(mutex->handle, INFINITE);
}

void unlock_mutex(struct mutex* mutex) {
	ReleaseMutex(mutex->handle);
}
