#pragma once

#include <corrosion/cr.h>

#include "entity.h"

struct mesh {
	struct vertex_buffer* vb;
	struct index_buffer* ib;
};

struct model {
	vector(struct mesh) meshes;
};

void init_model_from_fbx(struct model* model, const u8* raw, usize raw_size);
void deinit_model(struct model* model);

struct lit_vertex {
	v3f position;
};

struct renderer {
	table(struct mesh*, usize) drawlist;
	vector(m4f) transforms;
};

void register_renderer_resources();

struct renderer* new_renderer();
void free_renderer(struct renderer* renderer);

void renderer_begin(struct renderer* renderer);
void renderer_end(struct renderer* renderer);

void renderer_finalise(struct renderer* renderer);

void renderer_push(struct renderer* renderer, struct mesh* mesh, m4f transform);
