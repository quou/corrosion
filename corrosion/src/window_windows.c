#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <Windows.h>

#include "core.h"
#include "video.h"
#include "video_internal.h"
#include "video_vk.h"
#include "window.h"
#include "window_internal.h"

struct window window;

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
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_left] = true;
			window.pressed_mouse_btns[mouse_btn_left] = true;
			return 0;
		case WM_LBUTTONUP:
			window.held_mouse_btns[mouse_btn_left] = false;
			window.released_mouse_btns[mouse_btn_left] = true;
			return 0;
		case WM_MBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_middle] = true;
			window.pressed_mouse_btns[mouse_btn_middle] = true;
			return 0;
		case WM_MBUTTONUP:
			window.held_mouse_btns[mouse_btn_middle] = false;
			window.released_mouse_btns[mouse_btn_middle] = true;
			return 0;
		case WM_RBUTTONDOWN:
			window.held_mouse_btns[mouse_btn_right] = true;
			window.pressed_mouse_btns[mouse_btn_right] = true;
			return 0;
		case WM_RBUTTONUP:
			window.held_mouse_btns[mouse_btn_right] = false;
			window.released_mouse_btns[mouse_btn_right] = true;
			return 0;
		case WM_MOUSEWHEEL:
			window.scroll.y += GET_WHEEL_DELTA_WPARAM(wparam) > 1 ? 1 : -1;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	WNDCLASSW wc = { 0 };
	wc.hIcon = LoadIcon(null, IDI_APPLICATION);
	wc.hCursor = LoadCursor(null, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = GetModuleHandle(null);
	wc.lpfnWndProc = win32_event_callback;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszMenuName = null;
	wc.hbrBackground = null;
	wc.lpszClassName = L"corrosion";
	RegisterClassW(&wc);

	DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dw_style = WS_CAPTION | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_VISIBLE;

	window.resizable = config->resizable;
	if (window.resizable) {
		dw_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	}

	window.size = config->size;

	RECT win_rect = { 0, 0, window.size.x, window.size.y };
	AdjustWindowRectEx(&win_rect, dw_style, FALSE, dw_ex_style);
	i32 create_width = win_rect.right - win_rect.left;
	i32 create_height = win_rect.bottom - win_rect.top;

	window.hwnd = CreateWindowExA(dw_ex_style, "corrosion", config->title, dw_style, 0, 0,
			create_width, create_height, null, null, GetModuleHandle(null), null);

	/* For some reason this only sets the first character. Investigate. */
	SetWindowTextA(window.hwnd, config->title);

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
	table_set(window.keymap, VK_LSHIFT, key_shift);
	table_set(window.keymap, VK_RSHIFT, key_shift);
	table_set(window.keymap, VK_LCONTROL, key_control);
	table_set(window.keymap, VK_RCONTROL, key_control);
	table_set(window.keymap, VK_LWIN, key_super);
	table_set(window.keymap, VK_RWIN, key_super);
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

VkSurfaceKHR get_window_surface() {
	return window.surface;
}

void deinit_window() {
	core_free(window.raw_input_buffer);

	DestroyWindow(window.hwnd);

	free_table(window.keymap);
}

void update_events() {
	memset(window.pressed_keys, 0, sizeof window.pressed_keys);
	memset(window.released_keys, 0, sizeof window.released_keys);

	memset(window.pressed_mouse_btns, 0, sizeof window.pressed_mouse_btns);
	memset(window.released_mouse_btns, 0, sizeof window.released_mouse_btns);

	window.input_string_len = 0;
	window.scroll = make_v2i(0, 0);

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

		if (window.api == video_api_vulkan) {
			video_vk_want_recreate();
		}
	}
}

bool window_open() {
	return window.open;
}

v2i get_window_size() {
	return window.size;
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

v2i get_mouse_pos() {
	return window.mouse_pos;
}

v2i get_scroll() {
	return window.scroll;
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

		//ShowCursor(false);
	} else {
		RAWINPUTDEVICE rid = {
			.usUsagePage = 0x01,       /* HID_USAGE_PAGE_GENERIC */
			.usUsage = 0x02,           /* HID_USAGE_GENERIC_MOUSE */
			.dwFlags = RIDEV_REMOVE,
			.hwndTarget = 0
		};

		RegisterRawInputDevices(&rid, 1, sizeof rid);
	
		//ShowCursor(true);
	}
}

bool mouse_locked() {
	return window.mouse_locked;
}

bool get_input_string(const char** string, usize* len) {
	if (window.input_string_len > 0) {
		*string = window.input_string;
		*len = window.input_string_len;
		return true;
	}

	return false;
}