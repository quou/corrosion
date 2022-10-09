#pragma once

#include "common.h"
#include "maths.h"
#include "video.h"

struct ui;
struct ui_stylesheet;

void ui_init();
void ui_deinit();

struct ui* new_ui(const struct framebuffer* framebuffer);
void free_ui(struct ui* ui);

struct ui_renderer* get_ui_renderer(const struct ui* ui);

struct ui_stylesheet* load_stylesheet(const char* path, struct resource* r);

void ui_begin(struct ui* ui);
void ui_end(struct ui* ui);

void ui_draw_rect(struct ui* ui, v2f position, f32 z, v2f dimensions, v4f colour, f32 radius);
void ui_draw_gradient(struct ui* ui, v2f position, f32 z, v2f dimensions, v4f top_left, v4f top_right, v4f bot_left, v4f bot_right, f32 radius);
void ui_draw_circle(struct ui* ui, v2f position, f32 z, f32 radius, v4f colour);
void ui_draw_text(struct ui* ui, v2f position, f32 z, v2f dimensions, const char* text, v4f colour);
void ui_draw_texture(struct ui* ui, v2f position, f32 z, v2f dimensions, const struct texture* texture, v4i rect, v4f colour, f32 radius);
void ui_clip(struct ui* ui, v4f rect);

v2f ui_get_cursor_pos(const struct ui* ui);
void ui_set_cursor_pos(struct ui* ui, v2f pos);
void ui_advance(struct ui* ui, v2f dimensions);

f32 ui_advance_z(struct ui* ui);

void ui_font(struct ui* ui, struct font* font);
void ui_stylesheet(struct ui* ui, struct ui_stylesheet* ss);

void ui_begin_container_ex(struct ui* ui, const char* klass, v4f rect, bool scrollable);
void ui_begin_floating_container_ex(struct ui* ui, const char* klass, v4f rect, bool scrollable, f32 z);
#define ui_begin_container(ui_, r_, s_) ui_begin_container_ex(ui_, "", r_, s_)
#define ui_begin_floating_container(ui_, r_, s_) ui_begin_floating_container_ex(ui_, "", r_, s_, ui_advance_z(ui_))
void ui_end_container(struct ui* ui);

void ui_columns(struct ui* ui, usize count, f32* columns);

bool ui_label_ex(struct ui* ui, const char* klass, const char* text);
#define ui_label(ui_, t_) ui_label_ex(ui_, "", t_)
#define ui_button(ui_, t_) ui_label_ex(ui_, "button", t_)
#define ui_button_ex(ui_, c_, t_) ui_label_ex(ui_, "button " c_, t_)

void ui_linebreak(struct ui* ui);

bool ui_knob_ex(struct ui* ui, const char* klass, f32* val, f32 min, f32 max);
#define ui_knob(ui_, v_, mi, ma_) ui_knob_ex(ui_, "", v_, mi, ma_)

bool ui_picture_ex(struct ui* ui, const char* klass, const struct texture* texture, v4i rect);
#define ui_picture(ui_, t_) ui_picture_ex(ui_, "", t_, make_v4i(0, 0, video.get_texture_size(t_).x, video.get_texture_size(t_).y))

typedef bool (*ui_input_filter)(char c);

bool ui_text_input_filter(char c);
bool ui_number_input_filter(char c);
bool ui_alphanum_input_filter(char c);

bool ui_input_ex2(struct ui* ui, const char* klass, char* buf, usize buf_size, ui_input_filter filter, u64 id, bool is_password);
#define ui_password_input(ui_, b_, s_) ui_input_ex2(ui_, "", b_, s_, ui_text_input_filter, 0, true)
#define ui_input_ex(ui_, c_, b_, s_) ui_input_ex2(ui_, c_, b_, s_, ui_text_input_filter, 0, false)
#define ui_input(ui_, b_, s_) ui_input_ex(ui_, "", b_, s_)

bool ui_number_input_ex(struct ui* ui, const char* klass, f64* target);
#define ui_number_input(ui_, t_) ui_number_input_ex(ui_, "", t_)

bool ui_selectable_tree_node_ex(struct ui* ui, const char* klass, const char* text, bool leaf, bool* selected, u64 id);
#define ui_selectable_tree_node(ui_, t_, l_, s_) ui_selectable_tree_node_ex(ui_, "", t_, l_, s_, 0)
#define ui_tree_node_ex(ui_, c_, t_, l_, i_) ui_selectable_tree_node_ex(ui_, c_, t_, l_, null, i_)
#define ui_tree_node(ui_, t_, l_) ui_selectable_tree_node_ex(ui_, "", t_, l_, null, 0)
void ui_tree_pop(struct ui* ui);

bool ui_combo_ex(struct ui* ui, const char* klass, i32* item, const char** items, usize item_count, u64 id);
#define ui_combo(ui_, i_, is_, ic_) ui_combo_ex(ui_, "", i_, is_, ic_, 0)

bool ui_colour_picker_ex(struct ui* ui, const char* klass, v4f* colour, u64 id);
#define ui_colour_picker(ui_, c_) ui_colour_picker_ex(ui_, "", c_, 0)

bool ui_anything_hovered(struct ui* ui);

void ui_draw(const struct ui* ui);
