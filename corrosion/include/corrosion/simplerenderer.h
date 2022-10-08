#pragma once

/* Simple 2-D batched renderer. */
#include "atlas.h"
#include "common.h"
#include "font.h"
#include "video.h"

#define simple_renderer_verts_per_quad 4
#define simple_renderer_indices_per_quad 6

struct simple_renderer_vertex {
	v2f position;
	v2f uv;
	v4f colour;
	float use_texture;
};

struct simple_renderer {
	struct shader* shader;
	const struct framebuffer* framebuffer;

	struct vertex_buffer* vb;
	struct index_buffer* ib;
	struct pipeline* pipeline;

	struct text_renderer text_renderer;

	v4i clip;

	struct atlas* atlas;

	usize count;
	usize offset;
	usize max;

	struct {
		m4f projection;
	} vertex_ub;
};

struct simple_renderer_quad {
	v2f position;
	v2f dimensions;
	v4f colour;
	v4f rect;
	const struct texture* texture;
};

struct simple_renderer_text {
	v2f position;
	const char* text;
	v4f colour;
	struct font* font;
};

struct simple_renderer* new_simple_renderer(const struct framebuffer* framebuffer);
void free_simple_renderer(struct simple_renderer* renderer);
void simple_renderer_push(struct simple_renderer* renderer, const struct simple_renderer_quad* quad);
void simple_renderer_flush(struct simple_renderer* renderer);
void simple_renderer_end_frame(struct simple_renderer* renderer);
void simple_renderer_clip(struct simple_renderer* renderer, v4i clip);
void simple_renderer_push_text(struct simple_renderer* renderer, const struct simple_renderer_text* text);
