#pragma once

#include <corrosion/cr.h>

#include "entity.h"

struct mesh {
	struct vertex_buffer* vb;
	struct index_buffer* ib;

	usize count;
};

struct model {
	vector(struct mesh) meshes;
};

void init_model_from_fbx(struct model* model, const u8* raw, usize raw_size);
void deinit_model(struct model* model);

struct mesh_vertex {
	v3f position;
	v3f normal;
};

struct renderer {
	table(struct mesh*, usize) drawlist;
	vector(m4f) transforms;

	struct {
		m4f view, projection;
	} vertex_config;

	struct {
		v3f camera_pos;
	} fragment_config;

	m4f ub_transforms[1000];

	struct pipeline* pipeline;

	const struct framebuffer* framebuffer;
};

void register_renderer_resources();
struct model* load_model(const char* filename);

struct renderer* new_renderer(const struct framebuffer* framebuffer);
void free_renderer(struct renderer* renderer);

void renderer_begin(struct renderer* renderer);
void renderer_end(struct renderer* renderer);

void renderer_finalise(struct renderer* renderer, struct camera* camera);

void renderer_push(struct renderer* renderer, struct mesh* mesh, m4f transform);
