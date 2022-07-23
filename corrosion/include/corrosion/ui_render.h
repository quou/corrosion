#pragma once

/* Simple 2-D batched renderer. */
#include "atlas.h"
#include "common.h"
#include "font.h"
#include "video.h"

#define ui_renderer_batch_size 10000
#define ui_renderer_verts_per_quad 4
#define ui_renderer_indices_per_quad 6

struct ui_renderer_vertex {
	v2f position;
	v2f uv;
	v4f colour;
	f32 use_texture;
	f32 radius;
	v4f rect;
	f32 z;
};

struct ui_renderer {
	const struct shader* shader;
	const struct framebuffer* framebuffer;

	struct vertex_buffer* vb;
	struct index_buffer* ib;
	struct pipeline* pipeline;

	struct text_renderer text_renderer;

	v4i clip;

	struct atlas* atlas;

	usize count;
	usize offset;

	f32 text_z;

	struct {
		m4f projection;
	} vertex_ub;
};

struct ui_renderer_quad {
	v2f position;
	v2f dimensions;
	v4f colour;
	v4f rect;
	f32 radius;
	f32 z;
	const struct texture* texture;
};

struct ui_renderer_gradient_quad {
	struct {
		v4f top_left;
		v4f top_right;
		v4f bot_left;
		v4f bot_right;
	} colours;

	v2f position;
	v2f dimensions;
	f32 radius;
	v4f rect;
	f32 z;
	const struct texture* texture;
};

struct ui_renderer_text {
	v2f position;
	const char* text;
	v4f colour;
	struct font* font;
	f32 z;
};

struct ui_renderer* new_ui_renderer(const struct framebuffer* framebuffer);
void free_ui_renderer(struct ui_renderer* renderer);
void ui_renderer_push(struct ui_renderer* renderer, const struct ui_renderer_quad* quad);
void ui_renderer_push_gradient(struct ui_renderer* renderer, const struct ui_renderer_gradient_quad* quad);
void ui_renderer_flush(struct ui_renderer* renderer);
void ui_renderer_end_frame(struct ui_renderer* renderer);
void ui_renderer_clip(struct ui_renderer* renderer, v4i clip);
void ui_renderer_push_text(struct ui_renderer* renderer, const struct ui_renderer_text* text);
