#pragma once

#include <stdalign.h>

#include <corrosion/cr.h>

#include "entity.h"

struct mesh {
	struct vertex_buffer* vb;
	struct index_buffer* ib;

	vector(usize) instances;

	usize count;
};

struct model_node {
	/* Transforms the node into world space from geometry space. */
	m4f transform;
};

struct model {
	vector(struct mesh) meshes;
	vector(struct model_node) nodes;
};

void init_model_from_fbx(struct model* model, const u8* raw, usize raw_size);
void deinit_model(struct model* model);

struct mesh_vertex {
	v3f position;
	v3f normal;
};

struct mesh_instance {
	usize count;
	struct vertex_vector data;
};

struct mesh_instance_data {
	v4f r0;
	v4f r1;
	v4f r2;
	v4f r3;
};

struct renderer {
	table(struct mesh*, struct mesh_instance) drawlist;

	struct {
		m4f view, projection;
	} vertex_config;

	struct {
		v3f camera_pos;
	} fragment_config;

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
