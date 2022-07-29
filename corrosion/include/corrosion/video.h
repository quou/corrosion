#pragma once

#include "core.h"
#include "res.h"
#include "window.h"

enum {
	video_api_none = 0,
	video_api_vulkan
};

enum {
	framebuffer_attachment_colour,
	framebuffer_attachment_depth
};

enum {
	framebuffer_format_depth,
	framebuffer_format_rgba8i,
	framebuffer_format_rgba16f,
	framebuffer_format_rgba32f
};

enum {
	framebuffer_flags_fit      = 1 << 0,
	framebuffer_flags_default  = 1 << 1,
	framebuffer_flags_headless = 1 << 2
};

struct framebuffer_attachment_desc {
	u32 type;
	u32 format;
	v4f clear_colour;
};

struct framebuffer;

enum {
	pipeline_flags_none              = 1 << 0,
	pipeline_flags_depth_test        = 1 << 1,
	pipeline_flags_cull_back_face    = 1 << 2,
	pipeline_flags_cull_front_face   = 1 << 3,
	pipeline_flags_wo_clockwise      = 1 << 4,
	pipeline_flags_wo_anti_clockwise = 1 << 5,
	pipeline_flags_blend             = 1 << 6,
	pipeline_flags_dynamic_scissor   = 1 << 7,
	pipeline_flags_draw_lines        = 1 << 8,
	pipeline_flags_draw_line_strip   = 1 << 9,
	pipeline_flags_draw_tris         = 1 << 10
};

enum {
	pipeline_stage_vertex,
	pipeline_stage_fragment
};

enum {
	pipeline_attribute_float,
	pipeline_attribute_vec2,
	pipeline_attribute_vec3,
	pipeline_attribute_vec4,
};

struct pipeline_attribute {
	const char* name;
	u32 location;
	usize offset;
	u32 type;
};

struct pipeline_attributes {
	const struct pipeline_attribute* attributes;
	usize count;
	usize stride;
};

enum {
	pipeline_resource_uniform_buffer,
	pipeline_resource_framebuffer,
	pipeline_resource_texture
};

struct pipeline_resource {
	u32 type;

	union {
		struct {
			usize size;
		} uniform;

		const struct texture* texture;

		struct {
			struct framebuffer* ptr;
			usize attachment;
		} framebuffer;
	};
};

struct pipeline_descriptor {
	const char* name;
	u32 binding;
	u32 stage;
	struct pipeline_resource resource;
};

struct pipeline_descriptor_set {
	const char* name;
	const struct pipeline_descriptor* descriptors;
	usize count;
};

struct pipeline_descriptor_sets {
	const struct pipeline_descriptor_set* sets;
	usize count;
};

enum {
	texture_flags_none           = 1 << 0,
	texture_flags_filter_linear  = 1 << 1,
	texture_flags_filter_none    = 1 << 2,
};

enum {
	vertex_buffer_flags_none              = 1 << 0,
	vertex_buffer_flags_dynamic           = 1 << 1,
};

enum {
	index_buffer_flags_none              = 1 << 0,
	index_buffer_flags_u16               = 1 << 1,
	index_buffer_flags_u32               = 1 << 2
};

struct camera {
	v3f position;
	v3f rotation;

	f32 fov;
	f32 near_plane;
	f32 far_plane;
};

m4f get_camera_view(const struct camera* camera);
m4f get_camera_projection(const struct camera* camera);

struct pipeline;
struct vertex_buffer;
struct index_buffer;
struct shader;
struct texture;

struct video {
	u32 api;

	void (*init)(bool enable_validation, v4f clear_colour);
	void (*deinit)();

	void (*begin)();
	void (*end)();

	struct framebuffer* (*get_default_fb)();

	/* Framebuffer. */
	struct framebuffer* (*new_framebuffer)(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
	void (*free_framebuffer)(struct framebuffer* fb);
	v2i (*get_framebuffer_size)(const struct framebuffer* fb);
	void (*resize_framebuffer)(struct framebuffer* fb, v2i new_size);
	void (*begin_framebuffer)(struct framebuffer* fb);
	void (*end_framebuffer)(struct framebuffer* fb);
	struct texture* (*get_current_framebuffer_texture)(struct framebuffer* fb);

	/* Pipeline. */
	struct pipeline* (*new_pipeline)(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
		struct pipeline_attributes attributes, struct pipeline_descriptor_sets descriptor_sets);
	void (*free_pipeline)(struct pipeline* pipeline);
	void (*begin_pipeline)(const struct pipeline* pipeline);
	void (*end_pipeline)(const struct pipeline* pipeline);
	void (*recreate_pipeline)(struct pipeline* pipeline);
	void (*update_pipeline_uniform)(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
	void (*bind_pipeline_descriptor_set)(struct pipeline* pipeline, const char* set, usize target);
	void (*pipeline_add_descriptor_set)(struct pipeline* pipeline, const struct pipeline_descriptor_set* set);
	void (*pipeline_change_shader)(struct pipeline* pipeline, const struct shader* shader);

	/* Vertex buffer. */
	struct vertex_buffer* (*new_vertex_buffer)(void* verts, usize size, u32 flags);
	void (*free_vertex_buffer)(struct vertex_buffer* vb);
	void (*bind_vertex_buffer)(const struct vertex_buffer* vb);
	void (*update_vertex_buffer)(struct vertex_buffer* vb, const void* data, usize size, usize offset);

	/* Index buffer. */
	struct index_buffer* (*new_index_buffer)(void* elements, usize count, u32 flags);
	void (*free_index_buffer)(struct index_buffer* ib);
	void (*bind_index_buffer)(const struct index_buffer* ib);

	void (*draw)(usize count, usize offset, usize instances);
	void (*draw_indexed)(usize count, usize offset, usize instance);
	void (*set_scissor)(v4i rect);

	/* Texture. */
	struct texture* (*new_texture)(const struct image* image, u32 flags);
	void (*free_texture)(struct texture* texture);
	v2i  (*get_texture_size)(const struct texture* texture);
	void (*texture_copy)(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);

	/* Shader. */
	struct shader* (*new_shader)(const u8* data, usize data_size);
	void (*free_shader)(struct shader* shader);

	/* Maths. */
	m4f (*ortho)(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
	m4f (*persp)(f32 fov, f32 aspect, f32 near, f32 far);
};

extern struct video video;

void init_video(u32 api, bool enable_validation, v4f clear_colour);
void deinit_video();
