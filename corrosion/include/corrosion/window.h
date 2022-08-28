#pragma once

#include "app.h"
#include "common.h"
#include "maths.h"

struct window;

enum {
	window_event_size_changed = 0,
	window_event_mouse_moved,
	window_event_key_pressed,
	window_event_key_released,
	window_event_text_typed,
	window_event_mouse_button_pressed,
	window_event_mouse_button_released,
	window_event_any,
	window_event_count
};

struct window_event {
	u32 type;

	union {
		struct {
			v2i new_size;
		} size_changed;

		struct {
			v2i position;
		} mouse_moved;

		i32 key;
		i32 mouse_button;

		struct {
			const char* text;
			usize len;
		} text_typed;
	};
};

typedef void (*window_event_handler_t)(const struct window_event* event);

void init_window(const struct window_config* config, u32 api);
void deinit_window();
bool window_open();
void update_events();

void window_event_subscribe(u32 type, window_event_handler_t callback);
void window_event_unsubscribe(u32 type, window_event_handler_t callback);
void window_event_unsubscribe_all(u32 type);

v2i get_window_size();

enum {
	key_unknown = 0,
	key_A,
	key_B,
	key_C,
	key_D,
	key_E,
	key_F,
	key_G,
	key_H,
	key_I,
	key_J,
	key_K,
	key_L,
	key_M,
	key_N,
	key_O,
	key_P,
	key_Q,
	key_R,
	key_S,
	key_T,
	key_U,
	key_V,
	key_W,
	key_X,
	key_Y,
	key_Z,
	key_f1,
	key_f2,
	key_f3,
	key_f4,
	key_f5,
	key_f6,
	key_f7,
	key_f8,
	key_f9,
	key_f10,
	key_f11,
	key_f12,
	key_down,
	key_left,
	key_right,
	key_up,
	key_escape,
	key_return,
	key_backspace,
	key_tab,
	key_delete,
	key_home,
	key_end,
	key_page_up,
	key_page_down,
	key_insert,
	key_shift,
	key_control,
	key_super,
	key_alt,
	key_space,
	key_period,
	key_0,
	key_1,
	key_2,
	key_3,
	key_4,
	key_5,
	key_6,
	key_7,
	key_8,
	key_9,

	key_count
};

enum {
	mouse_btn_left = 0,
	mouse_btn_middle,
	mouse_btn_right,
	mouse_btn_count
};

bool key_pressed(u32 code);
bool key_just_pressed(u32 code);
bool key_just_released(u32 code);

bool mouse_btn_pressed(u32 code);
bool mouse_btn_just_pressed(u32 code);
bool mouse_btn_just_released(u32 code);

void lock_mouse(bool lock);
bool mouse_locked();

v2i get_mouse_pos();

v2i get_scroll();

bool get_input_string(const char** string, usize* len);
