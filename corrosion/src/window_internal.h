#pragma once

#include <vulkan/vulkan.h>

#include "core.h"
#include "maths.h"
#include "window.h"

#ifdef _WIN32

#include <Windows.h>

struct window {
	bool open;
	bool resizable;
	bool mouse_locked;

	u32 api;

	v2i size;
	v2i new_size;

	HWND hwnd;

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

	table(i32, u32) keymap;

	u8* raw_input_buffer;
	usize raw_input_buffer_capacity;
};

#else

#include <X11/X.h>
#include <X11/Xlib.h>

struct window {
	bool open;
	bool resizable;

	bool mouse_locked;

	u32 api;

	v2i size;

	Window window;

	VkSurfaceKHR surface;

	/* Context handle is a void pointer and not a GLXContext because
	 * including `GL/glx.h' will cause conflicts with Glad. */
	void* context;

	bool held_keys[key_count];
	bool pressed_keys[key_count];
	bool released_keys[key_count];

	bool held_mouse_btns[mouse_btn_count];
	bool pressed_mouse_btns[mouse_btn_count];
	bool released_mouse_btns[mouse_btn_count];

	v2i mouse_pos;
	v2i last_mouse;

	v2i warp_pos;

	v2i scroll;

	char input_string[256];
	usize input_string_len;

	table(KeySym, u32) keymap;
};

#endif

void window_create_vk_surface(VkInstance instance);
void window_destroy_vk_surface(VkInstance instance);
void window_get_vk_extensions(vector(const char*)* extensions);

VkSurfaceKHR get_window_vk_surface();

void window_create_gl_context();
void window_destroy_gl_context();
void* window_get_gl_proc(const char* name);
