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
		case WM_DESTROY:
			window.open = false;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void init_window(const struct window_config* config, u32 api) {
	memset(&window, 0, sizeof window);

	WNDCLASSW wc = { 0 };
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = win32_event_callback;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszMenuName = NULL;
	wc.hbrBackground = NULL;
	wc.lpszClassName = L"corrosion";
	RegisterClassW(&wc);

	DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dw_style = WS_CAPTION | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_VISIBLE;

	window.resizable = config->resizable;
	if (window.resizable) {
		dw_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	}

	window->size = config->size;

	RECT win_rect = { 0, 0, window.size.x, window.size.y };
	AdjustWindowRectEx(&win_rect, dw_style, FALSE, dw_ex_style);
	i32 create_width = win_rect.right - win_rect.left;
	i32 create_height = win_rect.bottom - win_rect.top;
}
