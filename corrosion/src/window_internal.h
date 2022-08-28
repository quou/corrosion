#pragma once

#include <vulkan/vulkan.h>

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

	table(i32, u32) keymap;
#elif defined(__linux__) || defined(__FreeBSD__)
	Window window;

	/* Context handle is a void pointer and not a GLXContext because
	 * including `GL/glx.h' will cause conflicts with Glad. */
	void* context;

	table(KeySym, u32) keymap;

	v2i last_mouse;
#endif

	VkSurfaceKHR surface;

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

void window_create_vk_surface(VkInstance instance);
void window_destroy_vk_surface(VkInstance instance);
void window_get_vk_extensions(vector(const char*)* extensions);

VkSurfaceKHR get_window_vk_surface();

void window_create_gl_context();
void window_destroy_gl_context();
void* window_get_gl_proc(const char* name);
void window_gl_swap();

void dispatch_event(const struct window_event* event);
