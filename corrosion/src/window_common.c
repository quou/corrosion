#include "window.h"
#include "window_internal.h"

struct window window;

bool window_open() {
	return window.open;
}

v2i get_window_size() {
	return window.size;
}

bool is_window_fullscreen() {
	return window.fullscreen;
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

void window_event_subscribe(u32 type, window_event_handler_t callback) {
	vector_push(window.event_handlers[type], callback);
}

void window_event_unsubscribe(u32 type, window_event_handler_t callback) {
	for (usize i = 0; i < vector_count(window.event_handlers[type]); i++) {
		if (window.event_handlers[type][i] == callback) {
			vector_delete(window.event_handlers[type], i);
		}
	}
}

void window_event_unsubscribe_all(u32 type) {
	vector_clear(window.event_handlers[type]);
}

void dispatch_event(const struct window_event* event) {
	for (usize i = 0; i < vector_count(window.event_handlers[event->type]); i++) {
		window.event_handlers[event->type][i](event);
	}

	for (usize i = 0; i < vector_count(window.event_handlers[window_event_any]); i++) {
		window.event_handlers[window_event_any][i](event);
	}
}
