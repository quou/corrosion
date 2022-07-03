#pragma once

#include "common.h"
#include "maths.h"
#include "video.h"

struct ui;

struct ui* new_ui(const struct framebuffer* framebuffer);
void free_ui(struct ui* ui);

void ui_begin(struct ui* ui);
void ui_end(struct ui* ui);

void ui_font(struct ui* ui, struct font* font);

void ui_begin_container_ex(struct ui* ui, const char* class, v4f rect);
#define ui_begin_container(ui_, r_) ui_begin_container_ex(ui_, "", r_)
void ui_end_container(struct ui* ui);

void ui_columns(struct ui* ui, usize count, f32* columns);

bool ui_label_ex(struct ui* ui, const char* class, const char* text);
#define ui_label(ui_, t_) ui_label_ex(ui_, "", t_)
#define ui_button(ui_, t_) ui_label_ex(ui_, "button", t_)

void ui_begin_window();
void ui_end_window();

void ui_draw(const struct ui* ui);
