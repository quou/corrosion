#define EGL_EGLEXT_PROTOTYPES

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

#include "core.h"
#include "window.h"
#include "window_internal.h"

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	window.canvas_name = "#canvas";
	emscripten_set_canvas_element_size(window.canvas_name, config->size.x, config->size.y);

	f64 x, y;
	emscripten_get_element_css_size(window.canvas_name, &x, &y);

	window.size.x = (i32)x;
	window.size.y = (i32)y;

	window.open = true;
	window.resizable = false;
	window.api = api;
}

i32 get_preferred_gpu_idx() {
	return -1;
}

#ifndef cr_no_opengl

void window_create_gl_context() {
	EmscriptenWebGLContextAttributes attribs;
	emscripten_webgl_init_context_attributes(&attribs);
	attribs.antialias = false;
	attribs.depth = true;
	attribs.premultipliedAlpha = false;
	attribs.stencil = true;
	attribs.majorVersion = 2;
	attribs.minorVersion = 0;
	attribs.enableExtensionsByDefault = true;

	window.context = emscripten_webgl_create_context(window.canvas_name, &attribs);
	if (!window.context) {
		abort_with("Failed to create WebGL context.");
	}

	emscripten_webgl_make_context_current(window.context);
}

void window_destroy_gl_context() {
	emscripten_webgl_destroy_context(window.context);
}

void* window_get_gl_proc(const char* name) {
	abort_with("Not implemented.");
	return null;
}

void window_gl_swap() {
	/* Nothing for Emscripten. */
}

void window_gl_set_swap_interval(i32 interval) {
	/* Nothing for Emscripten. */
}

bool is_opengl_supported() {
	return true;
}

#endif

void deinit_window() {
	/* TODO */
}

void update_events() {
	/* TODO */
}

void lock_mouse(bool lock) {
	abort_with("Not implemented.");
}
