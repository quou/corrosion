#ifndef cr_no_vulkan
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#endif

#include <Windows.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "shcore.lib")

#include "core.h"
#include "video.h"
#include "video_internal.h"
#include "video_vk.h"
#include "window.h"
#include "window_internal.h"

#ifndef cr_no_opengl
typedef void (*swap_interval_func)(i32 interval);

swap_interval_func wgl_swap_interval;
#endif

static LRESULT CALLBACK win32_event_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CLOSE:
			window.open = false;
			return 0;
		case WM_SIZE:
			i32 nw = lparam & 0xFFFF;
			i32 nh = (lparam >> 16) & 0xFFFF;

			window.new_size = make_v2i(nw, nh);

			return 0;
		case WM_MOUSEMOVE:
			if (!window.mouse_locked) {
				u32 x = lparam & 0xFFFF;
				u32 y = (lparam >> 16) & 0xFFFF;

				window.mouse_pos = make_v2i((i32)x, (i32)y);

				struct window_event mouse_event = {
					.type = window_event_mouse_moved,
					.mouse_moved.position = window.mouse_pos
				};

				dispatch_event(&mouse_event);

				return 0;
			}
			break;
		case WM_INPUT: {
			if (window.mouse_locked) {
				RECT rect;
				GetWindowRect(window.hwnd, &rect);
				rect.left   += window.size.x / 2 - 10;
				rect.right  -= window.size.x / 2 - 10;
				rect.top    += window.size.y / 2 - 10;
				rect.bottom -= window.size.y / 2 - 10;
				ClipCursor(&rect);

				UINT dw_size;

				GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dw_size, sizeof(RAWINPUTHEADER));

				if (dw_size > window.raw_input_buffer_capacity) {
					if (!window.raw_input_buffer_capacity) {
						window.raw_input_buffer_capacity = 8;
					}

					while (window.raw_input_buffer_capacity < dw_size) {
						window.raw_input_buffer_capacity *= 2;
					}

					window.raw_input_buffer = core_realloc(window.raw_input_buffer, window.raw_input_buffer_capacity);
				}

				if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, window.raw_input_buffer, &dw_size, sizeof(RAWINPUTHEADER)) != dw_size) {
					error("GetRawInputData does not return correct size !");
				}

				RAWINPUT* raw = (RAWINPUT*)window.raw_input_buffer;

				if (raw->header.dwType == RIM_TYPEMOUSE) {
					if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
						window.mouse_pos = v2i_add(window.mouse_pos, make_v2i(raw->data.mouse.lLastX, raw->data.mouse.lLastY));
					} else {
						window.mouse_pos = make_v2i(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
					}

					struct window_event mouse_event = {
						.type = window_event_mouse_moved,
						.mouse_moved.position = window.mouse_pos
					};

					dispatch_event(&mouse_event);
				}
			}
		} break;
		case WM_KEYDOWN: {
			i32 sym = (i32)wparam;

			i32* key_ptr = table_get(window.keymap, sym);
			if (key_ptr) {
				i32 key = *key_ptr;

				window.held_keys[key] = true;
				window.pressed_keys[key] = true;

				struct window_event key_event = {
					.type = window_event_key_pressed,
					.key = key
				};

				dispatch_event(&key_event);
			}

			/* Text input */
			char state[256];
			GetKeyboardState(state);

			/* This is unsafe and could potentially write past the bonuds of window.input_string.
			 * I doubt some madman will press more than 256 keys in one frame, though. */
			u32 scan_code = (lparam >> 16) & 0xff;
			i32 i = ToAscii((u32)wparam, scan_code, state, (LPWORD)window.input_string, 0);
			window.input_string[i] = '\0';

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

			return 0;
		}
		case WM_KEYUP: {
			i32 sym = (i32)wparam;

			i32* key_ptr = table_get(window.keymap, sym);
			if (key_ptr) {
				i32 key = *key_ptr;

				window.held_keys[key] = false;
				window.released_keys[key] = true;

				struct window_event key_event = {
					.type = window_event_key_released,
					.key = key
				};

				dispatch_event(&key_event);
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_left] = true;
			window.pressed_mouse_btns[mouse_btn_left] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_pressed,
				.mouse_button = mouse_btn_left
			});
			return 0;
		case WM_LBUTTONUP:
			window.held_mouse_btns[mouse_btn_left] = false;
			window.released_mouse_btns[mouse_btn_left] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_released,
				.mouse_button = mouse_btn_left
			});
			return 0;
		case WM_MBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_middle] = true;
			window.pressed_mouse_btns[mouse_btn_middle] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_pressed,
				.mouse_button = mouse_btn_middle
			});
			return 0;
		case WM_MBUTTONUP:
			window.held_mouse_btns[mouse_btn_middle] = false;
			window.released_mouse_btns[mouse_btn_middle] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_released,
				.mouse_button = mouse_btn_middle
			});
			return 0;
		case WM_RBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_right] = true;
			window.pressed_mouse_btns[mouse_btn_right] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_pressed,
				.mouse_button = mouse_btn_right
			});
			return 0;
		case WM_RBUTTONUP:
			window.held_mouse_btns[mouse_btn_right] = false;
			window.released_mouse_btns[mouse_btn_right] = true;

			dispatch_event(&(struct window_event) {
				.type = window_event_mouse_button_released,
				.mouse_button = mouse_btn_right
			});
			return 0;
		case WM_MOUSEWHEEL:
			window.scroll.y += GET_WHEEL_DELTA_WPARAM(wparam) > 1 ? 1 : -1;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	WNDCLASSA wc = { 0 };
	wc.hIcon = LoadIcon(null, IDI_APPLICATION);
	wc.hCursor = LoadCursor(null, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = GetModuleHandle(null);
	wc.lpfnWndProc = win32_event_callback;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszMenuName = null;
	wc.hbrBackground = null;
	wc.lpszClassName = "corrosion";
	RegisterClassA(&wc);

	DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dw_style = WS_CAPTION | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_VISIBLE;

	window.resizable = config->resizable;
	if (window.resizable) {
		dw_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	}

	RECT win_rect = { 0, 0, config->size.x, config->size.y };
	AdjustWindowRectEx(&win_rect, dw_style, FALSE, dw_ex_style);
	i32 create_width = win_rect.right - win_rect.left;
	i32 create_height = win_rect.bottom - win_rect.top;

	window.size = make_v2i(create_width, create_height);

	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	window.hwnd = CreateWindowExA(dw_ex_style, "corrosion", config->title, dw_style, 0, 0,
			create_width, create_height, null, null, GetModuleHandle(null), null);

	ShowWindow(window.hwnd, SW_SHOWNORMAL);

	memset(window.held_keys, 0, sizeof window.pressed_keys);
	memset(window.held_mouse_btns, 0, sizeof window.held_mouse_btns);

	memset(&window.keymap, 0, sizeof window.keymap);

	table_set(window.keymap, 0x00, key_unknown);
	table_set(window.keymap, 0x41, key_A);
	table_set(window.keymap, 0x42, key_B);
	table_set(window.keymap, 0x43, key_C);
	table_set(window.keymap, 0x44, key_D);
	table_set(window.keymap, 0x45, key_E);
	table_set(window.keymap, 0x46, key_F);
	table_set(window.keymap, 0x47, key_G);
	table_set(window.keymap, 0x48, key_H);
	table_set(window.keymap, 0x49, key_I);
	table_set(window.keymap, 0x4a, key_J);
	table_set(window.keymap, 0x4b, key_K);
	table_set(window.keymap, 0x4c, key_L);
	table_set(window.keymap, 0x4d, key_M);
	table_set(window.keymap, 0x4e, key_N);
	table_set(window.keymap, 0x4f, key_O);
	table_set(window.keymap, 0x50, key_P);
	table_set(window.keymap, 0x51, key_Q);
	table_set(window.keymap, 0x52, key_R);
	table_set(window.keymap, 0x53, key_S);
	table_set(window.keymap, 0x54, key_T);
	table_set(window.keymap, 0x55, key_U);
	table_set(window.keymap, 0x56, key_V);
	table_set(window.keymap, 0x57, key_W);
	table_set(window.keymap, 0x58, key_X);
	table_set(window.keymap, 0x59, key_Y);
	table_set(window.keymap, 0x5a, key_Z);
	table_set(window.keymap, VK_F1, key_f1);
	table_set(window.keymap, VK_F2, key_f2);
	table_set(window.keymap, VK_F3, key_f3);
	table_set(window.keymap, VK_F4, key_f4);
	table_set(window.keymap, VK_F5, key_f5);
	table_set(window.keymap, VK_F6, key_f6);
	table_set(window.keymap, VK_F7, key_f7);
	table_set(window.keymap, VK_F8, key_f8);
	table_set(window.keymap, VK_F9, key_f9);
	table_set(window.keymap, VK_F10, key_f10);
	table_set(window.keymap, VK_F11, key_f11);
	table_set(window.keymap, VK_F12, key_f12);
	table_set(window.keymap, VK_DOWN, key_down);
	table_set(window.keymap, VK_LEFT, key_left);
	table_set(window.keymap, VK_RIGHT, key_right);
	table_set(window.keymap, VK_UP, key_up);
	table_set(window.keymap, VK_ESCAPE, key_escape);
	table_set(window.keymap, VK_RETURN, key_return);
	table_set(window.keymap, VK_BACK, key_backspace);
	table_set(window.keymap, VK_RETURN, key_return);
	table_set(window.keymap, VK_TAB, key_tab);
	table_set(window.keymap, VK_DELETE, key_delete);
	table_set(window.keymap, VK_HOME, key_home);
	table_set(window.keymap, VK_END, key_end);
	table_set(window.keymap, VK_PRIOR, key_page_up);
	table_set(window.keymap, VK_NEXT, key_page_down);
	table_set(window.keymap, VK_INSERT, key_insert);
	table_set(window.keymap, 16, key_shift);
	table_set(window.keymap, 17, key_control);
	table_set(window.keymap, 91, key_super);
	table_set(window.keymap, VK_LMENU, key_alt);
	table_set(window.keymap, VK_RMENU, key_alt);
	table_set(window.keymap, VK_SPACE, key_space);
	table_set(window.keymap, 0x30, key_0);
	table_set(window.keymap, 0x31, key_1);
	table_set(window.keymap, 0x32, key_2);
	table_set(window.keymap, 0x33, key_3);
	table_set(window.keymap, 0x34, key_4);
	table_set(window.keymap, 0x35, key_5);
	table_set(window.keymap, 0x36, key_6);
	table_set(window.keymap, 0x37, key_7);
	table_set(window.keymap, 0x38, key_8);
	table_set(window.keymap, 0x39, key_9);

	window.open = true;
}

#ifndef cr_no_vulkan
void window_create_vk_surface(VkInstance instance) {
	if (vkCreateWin32SurfaceKHR(instance, &(VkWin32SurfaceCreateInfoKHR) {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hwnd = window.hwnd,
			.hinstance = GetModuleHandle(null)
		}, null, &window.surface) != VK_SUCCESS) {
		abort_with("Failed to create Vulkan surface.");
	}
}

void window_destroy_vk_surface(VkInstance instance) {
	vkDestroySurfaceKHR(instance, window.surface, null);
}

void window_get_vk_extensions(vector(const char*)* extensions) {
	vector_push(*extensions, "VK_KHR_surface");
	vector_push(*extensions, "VK_KHR_win32_surface");
}

VkSurfaceKHR get_window_vk_surface() {
	return window.surface;
}
#endif

void window_create_gl_context() {
	/* TODO: Create a modern context instead of a legacy one. */

	window.device_context = GetDC(window.hwnd);

	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nVersion = 1;
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	i32 pf;
	if (!(pf = ChoosePixelFormat(window.device_context, &pfd))) {
		abort_with("Error choosing pixel format.\n");
		return 0;
	}
	SetPixelFormat(window.device_context, pf, &pfd);

	if (!(window.render_context = wglCreateContext(window.device_context))) {
		abort_with("Failed to create OpenGL context.\n");
		return 0;
	}

	wglMakeCurrent(window.device_context, window.render_context);

	wgl_swap_interval = (swap_interval_func)wglGetProcAddress("wglSwapIntervalEXT");
	if (!wgl_swap_interval) {
		error("Failed to find the wglSwapIntervalEXT. The state of VSync thus cannot be changed.");
	}
}

void window_destroy_gl_context() {
	wglDeleteContext(window.render_context);
}

void* window_get_gl_proc(const char* name) {
	return wglGetProcAddress(name);
}

void window_gl_swap() {
	SwapBuffers(window.device_context);
}

void window_gl_set_swap_interval(i32 interval) {
	wgl_swap_interval(interval);
}

bool is_opengl_supported() {
	/* TODO */
	return true;
}

void deinit_window() {
	core_free(window.raw_input_buffer);

	DestroyWindow(window.hwnd);

	for (usize i = 0; i < window_event_count; i++) {
		free_vector(window.event_handlers[i]);
	}

	free_table(window.keymap);
}

void update_events() {
	memset(window.pressed_keys, 0, sizeof window.pressed_keys);
	memset(window.released_keys, 0, sizeof window.released_keys);

	memset(window.pressed_mouse_btns, 0, sizeof window.pressed_mouse_btns);
	memset(window.released_mouse_btns, 0, sizeof window.released_mouse_btns);

	window.input_string_len = 0;
	window.scroll = make_v2i(0, 0);

	ClipCursor(&(RECT) { 0, 0, -1, -1 });

	MSG msg;

	while (PeekMessage(&msg, null, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (window.size.x != window.new_size.x || window.size.y != window.new_size.y) {
		while (window.new_size.x == 0 || window.new_size.y == 0) {
			while (PeekMessage(&msg, null, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		window.size = window.new_size;

#ifndef cr_no_vulkan
		if (window.api == video_api_vulkan) {
			video_vk_want_recreate();
		}
#endif

		struct window_event resize_event = {
			.type = window_event_size_changed,
			.size_changed.new_size = window.size
		};

		dispatch_event(&resize_event);
	}
}

void lock_mouse(bool lock) {
	window.mouse_locked = lock;

	if (lock) {
		RAWINPUTDEVICE rid = {
			.usUsagePage = 0x01,       /* HID_USAGE_PAGE_GENERIC */
			.usUsage = 0x02,           /* HID_USAGE_GENERIC_MOUSE */
			.dwFlags = 0,
			.hwndTarget = 0
		};

		RegisterRawInputDevices(&rid, 1, sizeof rid);

		ShowCursor(false);
	} else {
		RAWINPUTDEVICE rid = {
			.usUsagePage = 0x01,       /* HID_USAGE_PAGE_GENERIC */
			.usUsage = 0x02,           /* HID_USAGE_GENERIC_MOUSE */
			.dwFlags = RIDEV_REMOVE,
			.hwndTarget = 0
		};

		RegisterRawInputDevices(&rid, 1, sizeof rid);
	
		ShowCursor(true);
	}
}
