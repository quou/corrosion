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
#include "video_gl.h"
#include "window.h"
#include "window_internal.h"

#ifndef cr_no_opengl
typedef BOOL (WINAPI* swap_interval_func)(INT);
typedef BOOL (WINAPI* choose_pixel_format_func)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef HGLRC (WINAPI* create_context_func)(HDC, HGLRC, const int*);

swap_interval_func wgl_swap_interval;
choose_pixel_format_func wgl_choose_pixel_format;
create_context_func wgl_create_context;

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001

#endif


static void request_api_recreate() {
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

	window.api = api;

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
	window.new_size = window.size;

	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	window.hwnd = CreateWindowExA(dw_ex_style, "corrosion", config->title, dw_style, 0, 0,
			create_width, create_height, null, null, GetModuleHandle(null), null);

	ShowWindow(window.hwnd, SW_SHOWNORMAL);

	set_window_fullscreen(config->fullscreen);

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

i32 get_preferred_gpu_idx() {
	return -1;
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
	/* Create a dummy window and context in order to load the WGL extensions. */
	HWND dummy_win = CreateWindowExW(
		0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, NULL);
	HDC dummy_dc = GetDC(dummy_win);


	PIXELFORMATDESCRIPTOR dummy_pfd = {
		.nSize = sizeof(dummy_pfd),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 24,
	};

	i32 format = ChoosePixelFormat(dummy_dc, &dummy_pfd);
	if (!format) {
		abort_with("Failed to choose pixel format.");
	}

	i32 r = DescribePixelFormat(dummy_dc, format, sizeof(dummy_pfd), &dummy_pfd);
	if (!r) {
		abort_with("Failed to describe OpenGL pixel format.");
	}

	if (!SetPixelFormat(dummy_dc, format, &dummy_pfd)) {
		abort_with("Failed to set the OpenGL pixel format.");
	}

	HGLRC dummy_rc = wglCreateContext(dummy_dc);
	if (!wglMakeCurrent(dummy_dc, dummy_rc)) {
		abort_with("Failed to make the context current.");
	}

	wgl_choose_pixel_format = wglGetProcAddress("wglChoosePixelFormatARB");
	wgl_create_context = wglGetProcAddress("wglCreateContextAttribsARB");
	wgl_swap_interval = wglGetProcAddress("wglSwapIntervalEXT");

	if (!wgl_choose_pixel_format || !wgl_create_context || !wgl_swap_interval) {
		abort_with("This computer is too old to run modern OpenGL.");
	}

	wglMakeCurrent(dummy_dc, null);
	wglDeleteContext(dummy_rc);
	ReleaseDC(dummy_win, dummy_dc);
	DestroyWindow(dummy_win);

	window.device_context = GetDC(window.hwnd);

	/* Create a modern context. */
	int pf_attribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, 1,
		WGL_SUPPORT_OPENGL_ARB, 1,
		WGL_DOUBLE_BUFFER_ARB,  1,
		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,     24,
		WGL_DEPTH_BITS_ARB,     24,
		WGL_STENCIL_BITS_ARB,   8,
		0,
	};

	u32 format_count;
	if (!wgl_choose_pixel_format(window.device_context, pf_attribs, null, 1, &format, &format_count) || format_count == 0) {
		abort_with("Unsupported pixel format.");
	}

	PIXELFORMATDESCRIPTOR pfd = { .nSize = sizeof(pfd) };
	r = DescribePixelFormat(window.device_context, format, sizeof(pfd), &pfd);
	if (!r) {
		abort_with("Failed to describe pixel format.");
	}

	if (!SetPixelFormat(window.device_context, format, &pfd)) {
		abort_with("Failed to set the OpenGL pixel format.");
	}

	int context_attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,

#ifdef debug
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
		0
	};

	window.render_context = wgl_create_context(window.device_context, null, context_attribs);
	if (!window.render_context) {
		abort_with("Failed to create an OpenGL context.");
	}

	r = wglMakeCurrent(window.device_context, window.render_context);
	if (!r) {
		abort_with("Failed to make the OpenGL context current.");
	}

	/* Test the context. */
	void* tf0 = window_get_gl_proc("glGetString");
	void* tf1 = window_get_gl_proc("glVertexAttribDivisor");
	if (!tf0 || !tf1) {
		abort_with("Non-functional OpenGL context.");
	}
}

void window_destroy_gl_context() {
	wglDeleteContext(window.render_context);
	ReleaseDC(window.hwnd, window.device_context);
}

void* window_get_gl_proc(const char* name) {
	void* p = (void*)wglGetProcAddress(name);
	if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1)) {
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void*)GetProcAddress(module, name);
	}

	return p;
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

		request_api_recreate();

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

bool get_clipboard_text(char* buf, usize buf_size) {
	HANDLE obj;
	LPSTR buffer;

	OpenClipboard(window.hwnd);

	obj = GetClipboardData(CF_TEXT);
	if (!obj) {
		CloseClipboard();
		return false;
	}

	buffer = GlobalLock(obj);
	if (!buffer) {
		CloseClipboard();
		return false;
	}

	GlobalUnlock(obj);

	usize buffer_len = strlen(buffer);
	buffer_len = cr_min(buffer_len, buf_size - 1);
	memcpy(buf, buffer, buffer_len);
	buf[buffer_len] = '\0';
	
	CloseClipboard();

	return true;
}

bool set_clipboard_text(const char* text, usize n) {
	HGLOBAL obj =  GlobalAlloc(GMEM_MOVEABLE, n + 1);
	memcpy(GlobalLock(obj), text, n + 1);

	GlobalUnlock(obj);
	OpenClipboard(window.hwnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, obj);
	CloseClipboard();

	return true;
}

void set_window_size(v2i size) {
	if (size.x <= 0 || size.y <= 0) { return; }
	
	window.new_size = make_v2i(size.x, size.y);

	SetWindowPos(window.hwnd, 0, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void set_window_title(const char* title) {
	SetWindowTextA(window.hwnd, title);
}

void set_window_fullscreen(bool fullscreen) {
	if (fullscreen == window.fullscreen) { return; }

	if (fullscreen) {
		POINT Point = { 0 };
		HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetMonitorInfo(Monitor, &MonitorInfo)) {
			window.old_size = window.size;

			DWORD Style = WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(window.hwnd, GWL_STYLE, Style);
			SetWindowPos(window.hwnd, 0, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

			window.new_size = make_v2i(
				MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top
			);
		}
	} else {
		DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

		if (window.resizable) {
			style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
		}

		SetWindowLongPtr(window.hwnd, GWL_STYLE, style);

		SetWindowPos(window.hwnd, 0,
			0, 0, window.old_size.x, window.old_size.y,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		set_window_size(window.old_size);
	}
	
	window.fullscreen = fullscreen;
}

bool init_temp_window_vk_surface(struct temp_window_vk_surface* s, VkInstance instance) {
	memset(s, 0, sizeof *s);

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
	wc.lpszClassName = "dummy window class";
	RegisterClassA(&wc);

	DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dw_style = WS_CAPTION | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_VISIBLE;

	s->hwnd = CreateWindowExA(dw_ex_style, "dummy window class", "Dummy window", dw_style, 0, 0,
			1, 1, null, null, GetModuleHandle(null), null);

	if (!s->hwnd) { return false; }

	if (vkCreateWin32SurfaceKHR(instance, &(VkWin32SurfaceCreateInfoKHR) {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hwnd = s->hwnd,
			.hinstance = GetModuleHandle(null)
		}, null, &s->surface) != VK_SUCCESS) {
		return false;
	}

	return true;
}

void deinit_temp_window_vk_surface(struct temp_window_vk_surface* s, VkInstance instance) {
	if (s->surface) {
		vkDestroySurfaceKHR(instance, s->surface, null);
	}

	if (s->hwnd) {
		DestroyWindow(s->hwnd);
	}
}

f32 get_dpi_scale() {
	/* TODO: */
	return 1.0f;
}
