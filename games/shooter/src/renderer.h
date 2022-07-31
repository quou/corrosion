#pragma once

#include <stdalign.h>

#include <corrosion/cr.h>

#define renderer_max_lights 500

struct mesh {
	struct vertex_buffer* vb;
	struct index_buffer* ib;

	vector(usize) instances;

	usize count;
};

struct light {
	f32 intensity;
	f32 range;
	v3f diffuse;
	v3f specular;
	v3f position;
};

struct light_std140 {
	alignas(4)  float intensity;
	alignas(4)  float range;
	alignas(16) v3f diffuse;
	alignas(16) v3f specular;
	alignas(16) v3f position;
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

	struct {
		alignas(4)  u32 light_count;
		alignas(16) v3f camera_pos;
		struct light_std140 lights[renderer_max_lights];
	} lighting_buffer;

	struct vertex_buffer* tri_vb;

	struct pipeline* lighting_pipeline;
	struct pipeline* pipeline;

	struct framebuffer* scene_fb;
	const struct framebuffer* target_fb;
};

void register_renderer_resources();
struct model* load_model(const char* filename);

struct renderer* new_renderer(const struct framebuffer* framebuffer);
void free_renderer(struct renderer* renderer);

void renderer_begin(struct renderer* renderer);
void renderer_end(struct renderer* renderer, struct camera* camera);

void renderer_finalise(struct renderer* renderer);

void renderer_push(struct renderer* renderer, struct mesh* mesh, struct material* material, m4f transform);
void renderer_push_light(struct renderer* renderer, const struct light* light);
