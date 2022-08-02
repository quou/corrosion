#define VK_USE_PLATFORM_XLIB_KHR

#include <vulkan/vulkan.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include "core.h"
#include "video.h"
#include "video_internal.h"
#include "video_vk.h"
#include "window.h"
#include "window_internal.h"

typedef GLXContext (*create_context_func)
	(Display*, GLXFBConfig, GLXContext, Bool, const int*);

Display* display;
Atom wm_delete_window_id, wm_protocols_id;

struct window window;

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	XInitThreads();

	display = XOpenDisplay(null);
	wm_protocols_id = XInternAtom(display, "WM_PROTOCOLS", False);
	wm_delete_window_id = XInternAtom(display, "WM_DELETE_WINDOW", False);

	window.api = api;

	window.size = config->size;
	window.open = true;
	window.resizable = config->resizable;

	Window root = DefaultRootWindow(display);

	XSetWindowAttributes attribs = {
		.event_mask =
			ExposureMask      |
			KeyPressMask      |
			KeyReleaseMask    |
			ButtonPressMask   |
			ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask
	};

	window.window = XCreateWindow(display, root, 0, 0, config->size.x, config->size.y,
		0, 0, InputOutput, XDefaultVisual(display, 0), CWEventMask, &attribs);
	XSetWMProtocols(display, window.window, &wm_delete_window_id, 1);

	XStoreName(display, window.window, config->title);

	XClearWindow(display, window.window);
	XMapRaised(display, window.window);

	memset(window.held_keys, 0, sizeof window.pressed_keys);
	memset(window.held_mouse_btns, 0, sizeof window.held_mouse_btns);

	memset(&window.keymap, 0, sizeof window.keymap);
	table_set(window.keymap, 0x00, key_unknown);
	table_set(window.keymap, 0x61, key_A);
	table_set(window.keymap, 0x62, key_B);
	table_set(window.keymap, 0x63, key_C);
	table_set(window.keymap, 0x64, key_D);
	table_set(window.keymap, 0x65, key_E);
	table_set(window.keymap, 0x66, key_F);
	table_set(window.keymap, 0x67, key_G);
	table_set(window.keymap, 0x68, key_H);
	table_set(window.keymap, 0x69, key_I);
	table_set(window.keymap, 0x6A, key_J);
	table_set(window.keymap, 0x6B, key_K);
	table_set(window.keymap, 0x6C, key_L);
	table_set(window.keymap, 0x6D, key_M);
	table_set(window.keymap, 0x6E, key_N);
	table_set(window.keymap, 0x6F, key_O);
	table_set(window.keymap, 0x70, key_P);
	table_set(window.keymap, 0x71, key_Q);
	table_set(window.keymap, 0x72, key_R);
	table_set(window.keymap, 0x73, key_S);
	table_set(window.keymap, 0x74, key_T);
	table_set(window.keymap, 0x75, key_U);
	table_set(window.keymap, 0x76, key_V);
	table_set(window.keymap, 0x77, key_W);
	table_set(window.keymap, 0x78, key_X);
	table_set(window.keymap, 0x79, key_Y);
	table_set(window.keymap, 0x7A, key_Z);
	table_set(window.keymap, XK_F1, key_f1);
	table_set(window.keymap, XK_F2, key_f2);
	table_set(window.keymap, XK_F3, key_f3);
	table_set(window.keymap, XK_F4, key_f4);
	table_set(window.keymap, XK_F5, key_f5);
	table_set(window.keymap, XK_F6, key_f6);
	table_set(window.keymap, XK_F7, key_f7);
	table_set(window.keymap, XK_F8, key_f8);
	table_set(window.keymap, XK_F9, key_f9);
	table_set(window.keymap, XK_F10, key_f10);
	table_set(window.keymap, XK_F11, key_f11);
	table_set(window.keymap, XK_F12, key_f12);
	table_set(window.keymap, XK_Down, key_down);
	table_set(window.keymap, XK_Left, key_left);
	table_set(window.keymap, XK_Right, key_right);
	table_set(window.keymap, XK_Up, key_up);
	table_set(window.keymap, XK_Escape, key_escape);
	table_set(window.keymap, XK_Return, key_return);
	table_set(window.keymap, XK_BackSpace, key_backspace);
	table_set(window.keymap, XK_Linefeed, key_return);
	table_set(window.keymap, XK_Tab, key_tab);
	table_set(window.keymap, XK_Delete, key_delete);
	table_set(window.keymap, XK_Home, key_home);
	table_set(window.keymap, XK_End, key_end);
	table_set(window.keymap, XK_Page_Up, key_page_up);
	table_set(window.keymap, XK_Page_Down, key_page_down);
	table_set(window.keymap, XK_Insert, key_insert);
	table_set(window.keymap, XK_Shift_L, key_shift);
	table_set(window.keymap, XK_Shift_R, key_shift);
	table_set(window.keymap, XK_Control_L, key_control);
	table_set(window.keymap, XK_Control_R, key_control);
	table_set(window.keymap, XK_Super_L, key_super);
	table_set(window.keymap, XK_Super_R, key_super);
	table_set(window.keymap, XK_Alt_L, key_alt);
	table_set(window.keymap, XK_Alt_R, key_alt);
	table_set(window.keymap, XK_space, key_space);
	table_set(window.keymap, XK_period, key_period);
	table_set(window.keymap, XK_0, key_0);
	table_set(window.keymap, XK_1, key_1);
	table_set(window.keymap, XK_2, key_2);
	table_set(window.keymap, XK_3, key_3);
	table_set(window.keymap, XK_4, key_4);
	table_set(window.keymap, XK_5, key_5);
	table_set(window.keymap, XK_6, key_6);
	table_set(window.keymap, XK_7, key_7);
	table_set(window.keymap, XK_8, key_8);
	table_set(window.keymap, XK_9, key_9);
}

void window_create_vk_surface(VkInstance instance) {
	if (vkCreateXlibSurfaceKHR(instance, &(VkXlibSurfaceCreateInfoKHR){
			.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
			.dpy = display,
			.window = window.window
		}, null, &window.surface) != VK_SUCCESS) {
		abort_with("Failed to create Vulkan surface.");
	}
}

void window_destroy_vk_surface(VkInstance instance) {
	vkDestroySurfaceKHR(instance, window.surface, null);
}

void window_get_vk_extensions(vector(const char*)* extensions) {
	vector_push(*extensions, "VK_KHR_surface");
	vector_push(*extensions, "VK_KHR_xlib_surface");
}

VkSurfaceKHR get_window_surface() {
	return window.surface;
}

void deinit_window() {
	XDestroyWindow(display, window.window);

	XCloseDisplay(display);

	free_table(window.keymap);
}

bool window_open() {
	return window.open;
}

v2i get_window_size() {
	return window.size;
}

void update_events() {
	memset(window.pressed_keys, 0, sizeof window.pressed_keys);
	memset(window.released_keys, 0, sizeof window.released_keys);

	memset(window.pressed_mouse_btns, 0, sizeof window.pressed_mouse_btns);
	memset(window.released_mouse_btns, 0, sizeof window.released_mouse_btns);

	window.input_string_len = 0;
	window.scroll = make_v2i(0, 0);

	XEvent e;

	while (XPending(display)) {
		XNextEvent(display, &e);
		switch (e.type) {
			case ClientMessage:
				if (e.xclient.message_type == wm_protocols_id && e.xclient.data.l[0] == wm_delete_window_id) {
					if (e.xclient.window == window.window) {
						window.open = false;
					}
				}
				break;
			case Expose: {
				XWindowAttributes gwa;
				XGetWindowAttributes(display, window.window, &gwa);
				window.size.x = gwa.width;
				window.size.y = gwa.height;

				if (window.api == video_api_vulkan) {
					video_vk_want_recreate();
				}
			} break;
			case KeyPress: {
				KeySym sym = XLookupKeysym(&e.xkey, 0);
				u32* key_ptr = table_get(window.keymap, sym);
				u32 key = key_unknown;

				if (key_ptr) {
					key = *key_ptr;
				}

				window.held_keys[key] = true;
				window.pressed_keys[key] = true;

				/* Text input. */
				XLookupString(&e.xkey, window.input_string, sizeof window.input_string, null, null);
				if ((window.input_string_len = strlen(window.input_string)) > 0) {
					for (usize i = 0; i < window.input_string_len; i++) {
						if (window.input_string[i] < ' ' || window.input_string[i] > '~') {
							window.input_string[i] = '\0';
							window.input_string_len--;
						}
					}
				}
			} break;
			case KeyRelease: {
				KeySym sym = XLookupKeysym(&e.xkey, 0);
				u32* key_ptr = table_get(window.keymap, sym);
				u32 key = key_unknown;

				if (key_ptr) {
					key = *key_ptr;
				}

				window.held_keys[key] = false;
				window.released_keys[key] = true;
			} break;
			case MotionNotify: {
				if (window.mouse_locked) {
					v2i centre = make_v2i(window.size.x / 2, window.size.y / 2);
					window.raw_pos = make_v2i(e.xmotion.x, e.xmotion.y);
					window.mouse_pos = v2i_add(v2i_sub(window.raw_pos, centre), window.mouse_pos);
				} else {
					window.mouse_pos = make_v2i(e.xmotion.x, e.xmotion.y);
				}
			} break;
			case ButtonPress: {
				switch (e.xbutton.button) {
					case 1:
					case 2:
					case 3:
						window.held_mouse_btns[e.xbutton.button - 1] = true;
						window.pressed_mouse_btns[e.xbutton.button - 1] = true;
						break;
					case 4:
						window.scroll.y++;
						break;
					case 5:
						window.scroll.y--;
						break;
					case 6:
						window.scroll.x++;
						break;
					case 7:
						window.scroll.x--;
						break;
				}
				break;
			} break;
			case ButtonRelease: {
				switch (e.xbutton.button) {
					case 1:
					case 2:
					case 3:
						window.held_mouse_btns[e.xbutton.button - 1] = false;
						window.released_mouse_btns[e.xbutton.button - 1] = true;
						break;
				}
				break;
			} break;
			default: break;
		}
	}

	if (window.mouse_locked) {
		XWarpPointer(display, None, window.window, 0, 0, 0, 0, window.size.x / 2, window.size.y / 2);
		XSync(display, False);
	}
}

bool key_pressed(u32 code) {
	return window.held_keys[code];
}

bool key_just_pressed(u32 code) {
	return window.pressed_keys[code];
}

bool key_just_released(u32 code) {
	return window.released_keys[code];
}

bool mouse_btn_pressed(u32 code) {
	return window.held_mouse_btns[code];
}

bool mouse_btn_just_pressed(u32 code) {
	return window.pressed_mouse_btns[code];
}

bool mouse_btn_just_released(u32 code) {
	return window.released_mouse_btns[code];
}

void lock_mouse(bool lock) {
	if (lock) {
		XColor col;
		char data[1] = {0X00};
		Pixmap blank = XCreateBitmapFromData(display, window.window, data, 1, 1);
		Cursor cursor = XCreatePixmapCursor(display, blank, blank, &col, &col, 0, 0);
		XDefineCursor(display, window.window, cursor);
		XFreePixmap(display, blank);
	} else {
		XUndefineCursor(display, window.window);
	}

	window.mouse_locked = lock;
}

bool mouse_locked() {
	return window.mouse_locked;
}

v2i get_mouse_pos() {
	return window.mouse_pos;
}

v2i get_scroll() {
	return window.scroll;
}

bool get_input_string(const char** string, usize* len) {
	if (window.input_string_len > 0) {
		*string = window.input_string;
		*len = window.input_string_len;
		return true;
	}

	return false;
}
