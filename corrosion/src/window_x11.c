#ifndef cr_no_vulkan
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <GL/glx.h>

#include "core.h"
#include "video.h"
#include "video_gl.h"
#include "video_internal.h"
#include "video_vk.h"
#include "window.h"
#include "window_internal.h"

#ifndef cr_no_opengl
typedef GLXContext (*create_context_func)
	(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef i32 (*swap_interval_func)(Display* dpy, GLXDrawable drawable, i32 interval);
swap_interval_func glx_swap_interval;
#endif

Display* display;
Atom wm_delete_window_id, wm_protocols_id;
Atom clipboard_at_id, targets_at_id, text_at_id, utf8_at_id;
Atom incr_at_id, xsel_at_id;
Atom XA_ATOM = 4, XA_STRING = 31;

static void update_window_size(v2i size) {
	if (size.x != window.size.x || size.y != window.size.y) {
		window.size = size;
#ifndef cr_no_vulkan
		if (window.api == video_api_vulkan) {
			video_vk_want_recreate();
		} else if (window.api == video_api_opengl) {
#endif
#ifndef cr_no_opengl
			video_gl_want_recreate();
#endif
#ifndef cr_no_vulkan
		}
#endif
	}

	struct window_event resize_event = {
		.type = window_event_size_changed,
		.size_changed.new_size = window.size
	};

	dispatch_event(&resize_event);
}


void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	XInitThreads();

	display = XOpenDisplay(null);
	wm_protocols_id = XInternAtom(display, "WM_PROTOCOLS", false);
	wm_delete_window_id = XInternAtom(display, "WM_DELETE_WINDOW", false);

	clipboard_at_id = XInternAtom(display, "CLIPBOARD", false);
	targets_at_id = XInternAtom(display, "TARGETS", false);
	text_at_id = XInternAtom(display, "TEXT", false);
	utf8_at_id = XInternAtom(display, "UTF8_STRING", true);
	incr_at_id = XInternAtom(display, "INCR", false);
	xsel_at_id = XInternAtom(display, "XSEL_DATA", false);

	if (utf8_at_id == None) { utf8_at_id = XA_STRING; }

	window.api = api;

	window.size = config->size;
	window.new_size = window.size;
	window.open = true;
	window.resizable = config->resizable;
	window.fullscreen = config->fullscreen;

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

	set_window_title(config->title);

	if (!config->resizable) {
		XSizeHints* hints = XAllocSizeHints();
		hints->flags = PMinSize | PMaxSize;
		hints->min_width = config->size.x;
		hints->min_height = config->size.y;
		hints->max_width = config->size.x;
		hints->max_height = config->size.y;

		XSetWMNormalHints(display, window.window, hints);

		XFree(hints);
	}

	if (window.fullscreen) {
		window.fullscreen = false;
		set_window_fullscreen(true);
	}

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

i32 get_preferred_gpu_idx() {
	char* dri_prime_str = getenv("DRI_PRIME");
	if (!dri_prime_str) { return -1; }

	return atoi(dri_prime_str);
}

void set_window_fullscreen(bool yes) {
	if (yes != window.fullscreen) {
		window.fullscreen = yes;
		Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
		Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
		XEvent xev = { 0 };
		xev.type = ClientMessage;
		xev.xclient.window = window.window;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = (window.fullscreen ? 1 : 0);
		xev.xclient.data.l[1] = fullscreen;
		xev.xclient.data.l[2] = 0;
		xev.xclient.data.l[3] = 0;
		XMapWindow(display, window.window);
		XSendEvent(display, DefaultRootWindow(display), False,
			SubstructureRedirectMask | SubstructureNotifyMask, &xev);
		XFlush(display);
		XWindowAttributes gwa;
		XGetWindowAttributes(display, window.window, &gwa);
		window.size.x = gwa.width;
		window.size.y = gwa.height;
		update_window_size(make_v2i(gwa.width, gwa.height));
	}
}

void set_window_title(const char* title) {
	XStoreName(display, window.window, title);
}

#ifndef cr_no_vulkan
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

VkSurfaceKHR get_window_vk_surface() {
	return window.surface;
}
#endif

#ifndef cr_no_opengl

void window_create_gl_context() {
	int visual_attribs[] = {
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER,  true,
		GLX_RED_SIZE,      8,
		GLX_GREEN_SIZE,    8,
		GLX_BLUE_SIZE,     8,
		GLX_DEPTH_SIZE,    24,
		GLX_STENCIL_SIZE,  8,
		None
	};

	i32 num_fbc = 0;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display),
			visual_attribs, &num_fbc);
	if (!fbc) {
		abort_with("Failed to create default OpenGL framebuffer.");
	}

	create_context_func create_context = (create_context_func)
			glXGetProcAddress((const u8*)"glXCreateContextAttribsARB");
	
	if (!create_context) {
		abort_with("Failed to find glXCreateContextAttribsARB.");
	}

	glx_swap_interval = (swap_interval_func)glXGetProcAddress("glXSwapIntervalEXT");
	if (!glx_swap_interval) {
		error("Cannot disable vsync, since glXSwapIntervalEXT was not found.");
	}

	i32 context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};

	window.context = create_context(display, fbc[0], null, true, context_attribs);
	if (!window.context) {
		abort_with("Failed to create OpenGL context");
	}

	glXMakeCurrent(display, window.window, window.context);

	XFree(fbc);
}

void window_destroy_gl_context() {
	glXDestroyContext(display, window.context);
}

void* window_get_gl_proc(const char* name) {
	return glXGetProcAddress((const u8*)name);
}

void window_gl_swap() {
	glXSwapBuffers(display, window.window);
}

void window_gl_set_swap_interval(i32 interval) {
	if (glx_swap_interval) {
		glx_swap_interval(display, window.window, interval);
	}
}

bool is_opengl_supported() {
	Display* dpy = XOpenDisplay(null);

	int visual_attribs[] = {
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER,  true,
		GLX_RED_SIZE,      8,
		GLX_GREEN_SIZE,    8,
		GLX_BLUE_SIZE,     8,
		GLX_DEPTH_SIZE,    24,
		GLX_STENCIL_SIZE,  8,
		None
	};

	i32 num_fbc = 0;
	GLXFBConfig* fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy),
			visual_attribs, &num_fbc);
	if (!fbc) {
		return false;
	}

	create_context_func create_context = (create_context_func)
			glXGetProcAddress((const u8*)"glXCreateContextAttribsARB");
	
	if (!create_context) {
		return false;
	}

	i32 context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};

	void* ctx = create_context(dpy, fbc[0], null, true, context_attribs);
	if (!ctx) {
		return false;
	}

	XFree(fbc);

	glXDestroyContext(dpy, ctx);
	XCloseDisplay(dpy);

	return true;
}

#endif

void deinit_window() {
	XDestroyWindow(display, window.window);

	XCloseDisplay(display);

	if (window.clipboard_text) { core_free(window.clipboard_text); }

	for (usize i = 0; i < window_event_count; i++) {
		free_vector(window.event_handlers[i]);
	}

	free_table(window.keymap);
}

void set_window_size(v2i size) {
	if (size.x <= 0 || size.y <= 0) { return; }

	XResizeWindow(display, window.window, size.x, size.y);
	update_window_size(size);
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

				set_window_size(make_v2i(gwa.width, gwa.height));
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

				struct window_event key_event = {
					.type = window_event_key_pressed,
					.key = key
				};

				dispatch_event(&key_event);

				/* Text input. */
				XLookupString(&e.xkey, window.input_string, sizeof window.input_string, null, null);
				if ((window.input_string_len = strlen(window.input_string)) > 0) {
					for (usize i = 0; i < window.input_string_len; i++) {
						if (window.input_string[i] < ' ' || window.input_string[i] > '~') {
							window.input_string[i] = '\0';
							window.input_string_len--;
						}
					}

					struct window_event text_event = {
						.type = window_event_text_typed,
						.text_typed.text = window.input_string,
						.text_typed.len = window.input_string_len
					};

					dispatch_event(&text_event);
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

				struct window_event key_event = {
					.type = window_event_key_released,
					.key = key
				};

				dispatch_event(&key_event);
			} break;
			case MotionNotify: {
				if (!window.mouse_locked) {
					v2i raw_pos = make_v2i(e.xmotion.x, e.xmotion.y);

					v2i delta = v2i_sub(raw_pos, window.last_mouse);

					window.mouse_pos = raw_pos;

					window.last_mouse = raw_pos;

					struct window_event mouse_event = {
						.type = window_event_mouse_moved,
						.mouse_moved.position = window.mouse_pos
					};

					dispatch_event(&mouse_event);
				}
			} break;
			case ButtonPress: {
				switch (e.xbutton.button) {
					case 1:
					case 2:
					case 3:
						window.held_mouse_btns[e.xbutton.button - 1] = true;
						window.pressed_mouse_btns[e.xbutton.button - 1] = true;

						struct window_event mouse_button_event = {
							.type = window_event_mouse_button_pressed,
							.mouse_button = e.xbutton.button - 1
						};

						dispatch_event(&mouse_button_event);
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

						struct window_event mouse_button_event = {
							.type = window_event_mouse_button_released,
							.mouse_button = e.xbutton.button - 1
						};

						dispatch_event(&mouse_button_event);

						break;
				}
				break;
			} break;
			case GenericEvent: {
				if (window.mouse_locked) {
					if (XGetEventData(display, &e.xcookie) && e.xcookie.evtype == XI_RawMotion) {
						XIRawEvent* re = e.xcookie.data;

						if (re->valuators.mask_len) {
							const double* values = re->raw_values;

							if (XIMaskIsSet(re->valuators.mask, 0)) {
								window.mouse_pos.x += (i32)*values;
								values++;
							}

							if (XIMaskIsSet(re->valuators.mask, 1)) {
								window.mouse_pos.y += (i32)*values;
							}
						}
						XFreeEventData(display, &e.xcookie);
					}
				}
			} break;
			case SelectionRequest: {
				if (e.xselectionrequest.selection != clipboard_at_id) { break; }

				XSelectionRequestEvent* xsr = &e.xselectionrequest;
				XSelectionEvent ev = {0};
				int R = 0;
				ev.type = SelectionNotify, ev.display = xsr->display, ev.requestor = xsr->requestor,
				ev.selection = xsr->selection, ev.time = xsr->time, ev.target = xsr->target, ev.property = xsr->property;

				if (ev.target == targets_at_id) {
					R = XChangeProperty(ev.display, ev.requestor, ev.property, XA_ATOM, 32,
						PropModeReplace, (unsigned char*)&utf8_at_id, 1);
				} else if (ev.target == XA_STRING || ev.target == text_at_id) {
					R = XChangeProperty(ev.display, ev.requestor, ev.property, XA_STRING, 8, PropModeReplace,
						window.clipboard_text, window.clipboard_text_len);
				} else if (ev.target == utf8_at_id) {
					R = XChangeProperty(ev.display, ev.requestor, ev.property, utf8_at_id, 8, PropModeReplace,
						window.clipboard_text, window.clipboard_text_len);
				} else {
					ev.property = None;
				}

				if ((R & 2) == 0) {
					XSendEvent(display, ev.requestor, 0, 0, (XEvent *)&ev);
				}

				break;
			} break;
			case SelectionNotify: {
				if (e.xselection.property) {
					char *result;
					u64 ressize, restail;
					i32 resbits;

					XGetWindowProperty(display, window.window, xsel_at_id, 0, 1024, False, AnyPropertyType,
						&utf8_at_id, &resbits, &ressize, &restail, (u8**)&result);

					if (utf8_at_id == incr_at_id) {
						XFree(result);
						break;
					} else {
						if (window.clipboard_text) {
							core_free(window.clipboard_text);
						}

						window.clipboard_text_len = ressize;
						window.clipboard_text = core_alloc(ressize + 1);

		 				memcpy(window.clipboard_text, result, ressize);
		 				window.clipboard_text[window.clipboard_text_len] = '\0';
					}

					XFree(result);
					break;
				}
			} break;
			default: break;
		}
	}
}

void lock_mouse(bool lock) {
	if (lock) {
		XColor col;
		char data[1] = {0x00};
		Pixmap blank = XCreateBitmapFromData(display, window.window, data, 1, 1);
		Cursor cursor = XCreatePixmapCursor(display, blank, blank, &col, &col, 0, 0);
		XDefineCursor(display, window.window, cursor);
		XGrabPointer(display, window.window, True,
			ButtonPressMask |
			ButtonReleaseMask |
			PointerMotionMask, GrabModeAsync, GrabModeAsync, window.window, None, CurrentTime);

    	XIEventMask em;
    	unsigned char mask[XIMaskLen(XI_RawMotion)] = { 0 };

    	em.deviceid = XIAllMasterDevices;
    	em.mask_len = sizeof(mask);
    	em.mask = mask;
    	XISetMask(mask, XI_RawMotion);

    	XISelectEvents(display, DefaultRootWindow(display), &em, 1);
	} else {
		XUndefineCursor(display, window.window);
		XUngrabPointer(display, CurrentTime);
	}

	window.mouse_locked = lock;
}

bool get_clipboard_text(char* buf, usize buf_size) {
	if (!buf) { return false; }

	usize len = cr_min(buf_size - 1, window.clipboard_text_len);
	memcpy(buf, window.clipboard_text, len);
	buf[len] = '\0';

	return true;
}

bool set_clipboard_text(const char* text, usize n) {
	XSetSelectionOwner(display, clipboard_at_id, window.window, 0);

	if (XGetSelectionOwner(display, clipboard_at_id) != window.window) {
		return false;
	}

	if (window.clipboard_text) { core_free(window.clipboard_text); }

	window.clipboard_text = core_alloc(n + 1);
	memcpy(window.clipboard_text, text, n);
	window.clipboard_text[n] = '\0';

	window.clipboard_text_len = n;

	return true;
}
