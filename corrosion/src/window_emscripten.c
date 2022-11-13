#define EGL_EGLEXT_PROTOTYPES

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

#include "core.h"
#include "window.h"
#include "window_internal.h"

extern void __cr_app_update();

static void main_loop() {
	__cr_app_update();

	window.input_string_len = 0;
	window.scroll = make_v2i(0, 0);

	memset(window.pressed_keys, 0, sizeof window.pressed_keys);
	memset(window.released_keys, 0, sizeof window.released_keys);

	memset(window.pressed_mouse_btns, 0, sizeof window.pressed_mouse_btns);
	memset(window.released_mouse_btns, 0, sizeof window.released_mouse_btns);
}

static i32 on_mouse_down_fun(i32 type, const EmscriptenMouseEvent* event, void* uptr) {
	u32 button;

	switch (event->button) {
		case 0: button = mouse_btn_left;   break;
		case 1: button = mouse_btn_middle; break;
		case 2: button = mouse_btn_right;  break;
		default: return false;
	}

	window.held_mouse_btns[button] = true;
	window.pressed_mouse_btns[button] = true;

	struct window_event mouse_button_event = {
		.type = window_event_mouse_button_pressed,
		.mouse_button = button
	};

	dispatch_event(&mouse_button_event);

	return true;
}

static i32 on_mouse_up_fun(i32 type, const EmscriptenMouseEvent* event, void* uptr) {
	u32 button;

	switch (event->button) {
		case 0: button = mouse_btn_left;   break;
		case 1: button = mouse_btn_middle; break;
		case 2: button = mouse_btn_right;  break;
		default: return false;
	}

	window.held_mouse_btns[button] = false;
	window.released_mouse_btns[button] = true;

	struct window_event mouse_button_event = {
		.type = window_event_mouse_button_released,
		.mouse_button = button
	};

	dispatch_event(&mouse_button_event);

	return true;
}

static i32 on_key_down_fun(i32 type, const EmscriptenKeyboardEvent* event, void* uptr) {
	u32* code_ptr = table_get(window.keymap, event->which);

	usize text_len = strlen(event->key);
	char c = event->key[0];

	bool consumed = false;

	if (text_len == 1 && c >= ' ' && c <= '~') {
		memcpy(window.input_string + window.input_string_len, &c, 1);
		window.input_string_len += 1;

		struct window_event text_event = {
			.type = window_event_text_typed,
			.text_typed.text = window.input_string,
			.text_typed.len = window.input_string_len
		};

		dispatch_event(&text_event);

		consumed = true;
	}

	if (!code_ptr) { return consumed; }

	window.held_keys[*code_ptr] = true;
	window.pressed_keys[*code_ptr] = true;

	struct window_event key_event = {
		.type = window_event_key_pressed,
		.key = *code_ptr
	};

	dispatch_event(&key_event);

	return true;
}

static i32 on_key_up_fun(i32 type, const EmscriptenKeyboardEvent* event, void* uptr) {
	u32* code_ptr = table_get(window.keymap, event->which);

	if (!code_ptr) { return false; }

	window.held_keys[*code_ptr] = false;
	window.released_keys[*code_ptr] = true;

	struct window_event key_event = {
		.type = window_event_key_released,
		.key = *code_ptr
	};

	dispatch_event(&key_event);

	return true;
}

static i32 on_mouse_move_fun(i32 type, const EmscriptenMouseEvent* event, void* uptr) {
	window.mouse_pos = make_v2i((i32)event->clientX, (i32)event->clientY);

	struct window_event mouse_event = {
		.type = window_event_mouse_moved,
		.mouse_moved.position = window.mouse_pos
	};

	dispatch_event(&mouse_event);

	return true;
}

static i32 on_scroll_fun(i32 type, const EmscriptenWheelEvent* event, void* uptr) {
	window.scroll.x += (i32)event->deltaX;
	window.scroll.y -= (i32)event->deltaY;

	return true;
}

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

	table_set(window.keymap, 0,   key_unknown);
	table_set(window.keymap, 65,  key_A);
	table_set(window.keymap, 66,  key_B);
	table_set(window.keymap, 67,  key_C);
	table_set(window.keymap, 68,  key_D);
	table_set(window.keymap, 69,  key_E);
	table_set(window.keymap, 70,  key_F);
	table_set(window.keymap, 71,  key_G);
	table_set(window.keymap, 72,  key_H);
	table_set(window.keymap, 73,  key_I);
	table_set(window.keymap, 74,  key_J);
	table_set(window.keymap, 75,  key_K);
	table_set(window.keymap, 76,  key_L);
	table_set(window.keymap, 77,  key_M);
	table_set(window.keymap, 78,  key_N);
	table_set(window.keymap, 79,  key_O);
	table_set(window.keymap, 80,  key_P);
	table_set(window.keymap, 81,  key_Q);
	table_set(window.keymap, 82,  key_R);
	table_set(window.keymap, 83,  key_S);
	table_set(window.keymap, 84,  key_T);
	table_set(window.keymap, 85,  key_U);
	table_set(window.keymap, 86,  key_V);
	table_set(window.keymap, 87,  key_W);
	table_set(window.keymap, 88,  key_X);
	table_set(window.keymap, 89,  key_Y);
	table_set(window.keymap, 90,  key_Z);
	table_set(window.keymap, 112, key_f1);
	table_set(window.keymap, 113, key_f2);
	table_set(window.keymap, 114, key_f3);
	table_set(window.keymap, 115, key_f4);
	table_set(window.keymap, 116, key_f5);
	table_set(window.keymap, 117, key_f6);
	table_set(window.keymap, 118, key_f7);
	table_set(window.keymap, 119, key_f8);
	table_set(window.keymap, 120, key_f9);
	table_set(window.keymap, 121, key_f10);
	table_set(window.keymap, 122, key_f11);
	table_set(window.keymap, 123, key_f12);
	table_set(window.keymap, 40,  key_down);
	table_set(window.keymap, 37,  key_left);
	table_set(window.keymap, 39,  key_right);
	table_set(window.keymap, 38,  key_up);
	table_set(window.keymap, 27,  key_escape);
	table_set(window.keymap, 13,  key_return);
	table_set(window.keymap, 8,   key_backspace);
	table_set(window.keymap, 13,  key_return);
	table_set(window.keymap, 9,   key_tab);
	table_set(window.keymap, 46,  key_delete);
	table_set(window.keymap, 36,  key_home);
	table_set(window.keymap, 35,  key_end);
	table_set(window.keymap, 33,  key_page_up);
	table_set(window.keymap, 34,  key_page_down);
	table_set(window.keymap, 45,  key_insert);
	table_set(window.keymap, 16,  key_shift);
	table_set(window.keymap, 17,  key_control);
	table_set(window.keymap, 91,  key_super);
	table_set(window.keymap, 18,  key_alt);
	table_set(window.keymap, 32,  key_space);
	table_set(window.keymap, 190, key_period);
	table_set(window.keymap, 48,  key_0);
	table_set(window.keymap, 49,  key_1);
	table_set(window.keymap, 50,  key_2);
	table_set(window.keymap, 51,  key_3);
	table_set(window.keymap, 52,  key_4);
	table_set(window.keymap, 53,  key_5);
	table_set(window.keymap, 54,  key_6);
	table_set(window.keymap, 55,  key_7);
	table_set(window.keymap, 56,  key_8);
	table_set(window.keymap, 57,  key_9);

	emscripten_set_mousemove_callback(window.canvas_name, null, true, on_mouse_move_fun);
	emscripten_set_mousedown_callback(window.canvas_name, null, true, on_mouse_down_fun);
	emscripten_set_mouseup_callback(window.canvas_name, null, true, on_mouse_up_fun);
	emscripten_set_keydown_callback(window.canvas_name, null, true, on_key_down_fun);
	emscripten_set_keyup_callback(window.canvas_name, null, true, on_key_up_fun);
	emscripten_set_wheel_callback(window.canvas_name, null, true, on_scroll_fun);
}

void window_emscripten_run() {
	emscripten_set_main_loop(main_loop, 0, true);
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
	
}

void window_gl_set_swap_interval(i32 interval) {

}

bool is_opengl_supported() {
	return true;
}

#endif

void deinit_window() {
	if (window.clipboard_text) { core_free(window.clipboard_text); }
}

void update_events() {

}

void lock_mouse(bool lock) {
	abort_with("Not implemented.");
}

bool get_clipboard_text(char* buf, usize buf_size) {
	if (!buf) { return false; }

	usize len = cr_min(buf_size - 1, window.clipboard_text_len);
	memcpy(buf, window.clipboard_text, len);
	buf[len] = '\0';

	return true;
}

bool set_clipboard_text(const char* text, usize n) {
	if (window.clipboard_text) { core_free(window.clipboard_text); }

	window.clipboard_text = core_alloc(n + 1);
	memcpy(window.clipboard_text, text, n);
	window.clipboard_text[n] = '\0';

	window.clipboard_text_len = n;

	return true;
}

void set_window_size(v2i size) {
	abort_with("Not implemented.");
}

void set_window_title(const char* title) {
	abort_with("Not implemented.");
}

void set_window_fullscreen(bool fullscreen) {
	abort_with("Not implemented.");
}
