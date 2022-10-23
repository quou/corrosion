#pragma once

#ifndef cr_no_vulkan
#include <vulkan/vulkan.h>
#endif

#include "core.h"
#include "maths.h"
#include "window.h"

extern struct window window;

#if defined(_WIN32)

#include <Windows.h>

#elif defined(__linux__) || defined(__FreeBSD__)

#include <X11/X.h>
#include <X11/Xlib.h>

#endif

struct window {
	bool open;
	bool resizable;
	bool mouse_locked;

	u32 api;

	v2i size;
	v2i new_size;

#if defined(_WIN32)
	HWND hwnd;

	HDC device_context;
	HGLRC render_context;

	table(i32, u32) keymap;
#elif defined(__linux__) || defined(__FreeBSD__)
	Window window;

#ifndef cr_no_opengl
	/* Context handle is a void pointer and not a GLXContext because
	 * including `GL/glx.h' will cause conflicts with Glad. */
	void* context;
#endif

	table(KeySym, u32) keymap;

	v2i last_mouse;
#endif

#ifndef cr_no_vulkan
	VkSurfaceKHR surface;
#endif

	bool held_keys[key_count];
	bool pressed_keys[key_count];
	bool released_keys[key_count];

	bool held_mouse_btns[mouse_btn_count];
	bool pressed_mouse_btns[mouse_btn_count];
	bool released_mouse_btns[mouse_btn_count];

	v2i mouse_pos;

	v2i scroll;

	char input_string[256];
	usize input_string_len;

	u8* raw_input_buffer;
	usize raw_input_buffer_capacity;

	vector(window_event_handler_t) event_handlers[window_event_count];
};

/* Returns the preferred GPU index. Returns
 * a negative value on failure or none
 * specified. */
i32 get_preferred_gpu_idx();

#ifndef cr_no_vulkan
void window_create_vk_surface(VkInstance instance);
void window_destroy_vk_surface(VkInstance instance);
void window_get_vk_extensions(vector(const char*)* extensions);

VkSurfaceKHR get_window_vk_surface();
#endif

#ifndef cr_no_opengl
void window_create_gl_context();
void window_destroy_gl_context();
void* window_get_gl_proc(const char* name);
void window_gl_swap();
void window_gl_set_swap_interval(i32 interval);
#endif

void dispatch_event(const struct window_event* event);
