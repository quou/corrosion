#pragma once

#include <stdalign.h>

#include <corrosion/cr.h>

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
	v2f uv;
};

struct mesh_instance {
	usize count;
	struct vertex_vector data;
};

struct material {
	v3f diffuse;
	struct texture* diffuse_map;
};

struct mesh_instance_data {
	v4f r0;
	v4f r1;
	v4f r2;
	v4f r3;
	v3f diffuse;
	v4f diffuse_rect;
};

struct renderer {
	table(struct mesh*, struct mesh_instance) drawlist;

	struct atlas* diffuse_atlas;

	struct {
		alignas(16) m4f view;
		alignas(16) m4f projection;
		alignas(8)  v2f atlas_size;
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

void renderer_push(struct renderer* renderer, struct mesh* mesh, struct material* material, m4f transform);
