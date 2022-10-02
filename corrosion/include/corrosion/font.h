#pragma once

#include "common.h"
#include "video.h"

/* Generic font rendering.
 *
 * Use the text_renderer struct with the draw_character function
 * hooked up to whatever renderer you would like to use.
 * 
 * The following example assumes you have a renderer set up to draw
 * 2D sprites and are using the resource manager. If you're using the
 * simple renderer or the UI renderer, these already have font rendering
 * setup using this exact API, so you can simply call simple_renderer_push_text
 * or ui_renderer_push_text.
 * 
 *     struct font* font = load_font("Arial.ttf", 14);
 * 
 *     static void draw_char(void*, const struct texture* atlas, v2f pos, v4f rect, v4f col) {
 *         render_sprite(atlas, pos, rect, col);
 *     }
 * 
 *     struct text_renderer tr = {
 *         .draw_character = draw_char
 *     }
 *
 *     render_text(&tr, font, "Hello, world!", make_v2f(0.0f, 0.0f), make_rgba(0xffffff, 255));
 */

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
v2f get_char_dimensions(struct font* font, const char* c);
v2f get_text_dimensions(struct font* font, const char* text);
v2f get_text_n_dimensions(struct font* font, const char* text, usize n);
f32 get_font_height(const struct font* font);
