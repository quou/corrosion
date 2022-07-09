#pragma once

#include "common.h"
#include "video.h"

struct font;

usize font_struct_size();
struct font* new_font(const u8* raw, usize raw_size, f32 size);
void free_font(struct font* font);

void init_font(struct font* font, const u8* raw, usize raw_size, f32 size);
void deinit_font(struct font* font);

void* get_font_data(struct font* font);

struct text_renderer {
	void* uptr;

	void (*draw_character)(void* uptr, const struct texture* atlas, v2f position, v4f rect, v4f colour);
};

void render_text(const struct text_renderer* renderer, struct font* font, const char* text, v2f position, v4f colour);
v2f get_char_dimentions(struct font* font, const char* c);
v2f get_text_dimentions(struct font* font, const char* text);
v2f get_text_n_dimentions(struct font* font, const char* text, usize n);
f32 get_font_height(const struct font* font);
