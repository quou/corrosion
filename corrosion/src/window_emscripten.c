#include "core.h"
#include "window.h"
#include "window_internal.h"

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	abort_with("Not implemented.");
}

i32 get_preferred_gpu_idx() {
	return -1;
}

#ifndef cr_no_opengl

void window_create_gl_context() {
	abort_with("Not implemented.");
}

void window_destroy_gl_context() {
	abort_with("Not implemented.");
}

void* window_get_gl_proc(const char* name) {
	abort_with("Not implemented.");
}

void window_gl_swap() {
	abort_with("Not implemented.");
}

void window_gl_set_swap_interval(i32 interval) {
	abort_with("Not implemented.");
}

bool is_opengl_supported() {
	abort_with("Not implemented.");
}

#endif

void deinit_window() {
	abort_with("Not implemented.");
}

void update_events() {
	abort_with("Not implemented.");
}

void lock_mouse(bool lock) {
	abort_with("Not implemented.");
}
