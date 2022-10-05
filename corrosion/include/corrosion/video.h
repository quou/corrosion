#pragma once

#include "app.h"
#include "core.h"
#include "res.h"
#include "window.h"

enum {
	video_api_none = 0,
	video_api_vulkan,
	video_api_opengl
};

enum {
	framebuffer_attachment_colour = 0,
	framebuffer_attachment_depth,
	framebuffer_attachment_type_count
};

enum {
	framebuffer_format_depth = 0,
	framebuffer_format_rgba8i,
	framebuffer_format_rgba16f,
	framebuffer_format_rgba32f,
	framebuffer_format_count
};

enum {
	framebuffer_flags_fit                     = 1 << 0,
	framebuffer_flags_default                 = 1 << 1,
	framebuffer_flags_headless                = 1 << 2,
	framebuffer_flags_attachments_are_storage = 1 << 3
};

struct framebuffer_attachment_desc {	
	const char* name;
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
	pipeline_flags_blend             = 1 << 4,
	pipeline_flags_dynamic_scissor   = 1 << 5,
	pipeline_flags_draw_lines        = 1 << 6,
	pipeline_flags_draw_line_strip   = 1 << 7,
	pipeline_flags_draw_tris         = 1 << 8,
	pipeline_flags_draw_points       = 1 << 9,
	pipeline_flags_compute           = 1 << 10
};

enum {
	pipeline_stage_vertex,
	pipeline_stage_fragment,
	pipeline_stage_compute
};

enum {
	pipeline_attribute_float,
	pipeline_attribute_vec2,
	pipeline_attribute_vec3,
	pipeline_attribute_vec4,
};

struct pipeline_config {
	f32 line_width;
};

struct pipeline_attribute {
	const char* name;
	u32 location;
	usize offset;
	u32 type;
};

struct pipeline_attributes {
	struct pipeline_attribute* attributes;
	usize count;
};

enum {
	pipeline_attribute_rate_per_vertex,
	pipeline_attribute_rate_per_instance
};

struct pipeline_attribute_binding {
	u32 binding;
	u32 rate;
	usize stride;
	struct pipeline_attributes attributes;
};

struct pipeline_attribute_bindings {
	struct pipeline_attribute_binding* bindings;
	usize count;
};

enum {
	pipeline_resource_uniform_buffer,
	pipeline_resource_texture,
	pipeline_resource_texture_storage,
	pipeline_resource_storage
};

struct pipeline_resource {
	u32 type;

	union {
		struct {
			usize size;
		} uniform;

		const struct texture* texture;
		const struct storage* storage;
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
	struct pipeline_descriptor* descriptors;
	usize count;
};

struct pipeline_descriptor_sets {
	struct pipeline_descriptor_set* sets;
	usize count;
};

enum {
	texture_flags_none           = 1 << 0,
	texture_flags_filter_linear  = 1 << 1,
	texture_flags_filter_none    = 1 << 2,
	texture_flags_storage        = 1 << 3,
	texture_flags_repeat         = 1 << 4,
	texture_flags_clamp          = 1 << 5
};

enum {
	texture_format_r8i,
	texture_format_r16f,
	texture_format_r32f,
	texture_format_rg8i,
	texture_format_rg16f,
	texture_format_rg32f,
	texture_format_rgb8i,
	texture_format_rgb16f,
	texture_format_rgb32f,
	texture_format_rgba8i,
	texture_format_rgba16f,
	texture_format_rgba32f,
	texture_format_count
};

enum {
	texture_state_shader_write = 0,
	texture_state_shader_graphics_read,
	texture_state_shader_compute_read,
	texture_state_shader_compute_sample,
	texture_state_attachment_write
};

enum {
	vertex_buffer_flags_none              = 1 << 0,
	vertex_buffer_flags_dynamic           = 1 << 1,
	vertex_buffer_flags_transferable      = 1 << 2
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

enum {
	storage_flags_none           = 1 << 0,
	storage_flags_cpu_writable   = 1 << 1,
	storage_flags_cpu_readable   = 1 << 2,
	storage_flags_vertex_buffer  = 1 << 3,
	storage_flags_index_buffer   = 1 << 4,
	storage_flags_16bit_indices  = 1 << 5,
	storage_flags_32bit_indices  = 1 << 6
};

enum {
	storage_bind_as_vertex_buffer = 0,
	storage_bind_as_index_buffer
};

enum {
	storage_state_compute_read = 0,
	storage_state_compute_write,
	storage_state_compute_read_write,
	storage_state_fragment_read,
	storage_state_fragment_write,
	storage_state_vertex_read,
	storage_state_vertex_write,
	storage_state_dont_care,
};

enum {
	video_feature_base    = 1 << 0,
	video_feature_compute = 1 << 1,
	video_feature_storage = 1 << 2,
	video_feature_barrier = 1 << 3
};

m4f get_camera_view(const struct camera* camera);
m4f get_camera_projection(const struct camera* camera, f32 aspect);

struct pipeline;
struct vertex_buffer;
struct index_buffer;
struct shader;
struct texture;
struct storage;

struct video {
	u32 api;

	void (*init)(const struct video_config* config);
	void (*deinit)();

	void (*begin)(bool present);
	void (*end)(bool present);

	struct framebuffer* (*get_default_fb)();

	/* Framebuffer. */
	struct framebuffer* (*new_framebuffer)(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
	void (*free_framebuffer)(struct framebuffer* fb);
	v2i (*get_framebuffer_size)(const struct framebuffer* fb);
	void (*resize_framebuffer)(struct framebuffer* fb, v2i new_size);
	void (*begin_framebuffer)(struct framebuffer* fb);
	void (*end_framebuffer)(struct framebuffer* fb);
	struct texture* (*get_attachment)(struct framebuffer* fb, u32 index);

	/* Pipeline. */
	struct pipeline* (*new_pipeline)(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
		struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets);
	struct pipeline* (*new_pipeline_ex)(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
		struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config);
	struct pipeline* (*new_compute_pipeline)(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets);
	void (*free_pipeline)(struct pipeline* pipeline);
	void (*begin_pipeline)(const struct pipeline* pipeline);
	void (*end_pipeline)(const struct pipeline* pipeline);
	void (*recreate_pipeline)(struct pipeline* pipeline);
	void (*update_pipeline_uniform)(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
	void (*bind_pipeline_descriptor_set)(struct pipeline* pipeline, const char* set, usize target);
	void (*pipeline_change_shader)(struct pipeline* pipeline, const struct shader* shader);

	/* Storage. */
	struct storage* (*new_storage)(u32 flags, usize size, void* initial_data);
	void (*update_storage)(struct storage* storage, void* data);
	void (*update_storage_region)(struct storage* storage, void* data, usize offset, usize size);
	void (*copy_storage)(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size);
	void (*storage_barrier)(struct storage* storage, u32 state);
	void (*storage_bind_as)(const struct storage* storage, u32 as, u32 point);
	void (*free_storage)(struct storage* storage);

	/* Vertex buffer. */
	struct vertex_buffer* (*new_vertex_buffer)(void* verts, usize size, u32 flags);
	void (*free_vertex_buffer)(struct vertex_buffer* vb);
	void (*bind_vertex_buffer)(const struct vertex_buffer* vb, u32 point);
	void (*update_vertex_buffer)(struct vertex_buffer* vb, const void* data, usize size, usize offset);
	void (*copy_vertex_buffer)(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size);

	/* Index buffer. */
	struct index_buffer* (*new_index_buffer)(void* elements, usize count, u32 flags);
	void (*free_index_buffer)(struct index_buffer* ib);
	void (*bind_index_buffer)(const struct index_buffer* ib);

	void (*draw)(usize count, usize offset, usize instances);
	void (*draw_indexed)(usize count, usize offset, usize instance);
	void (*set_scissor)(v4i rect);
	void (*invoke_compute)(v3u group_count);

	/* Texture. */
	struct texture* (*new_texture)(const struct image* image, u32 flags, u32 format);
	struct texture* (*new_texture_3d)(v3i size, u32 flags, u32 format);
	void (*free_texture)(struct texture* texture);
	v2i  (*get_texture_size)(const struct texture* texture);
	v3i  (*get_texture_3d_size)(const struct texture* texture);
	void (*texture_copy)(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);
	void (*texture_copy_3d)(struct texture* dst, v3i dst_offset, const struct texture* src, v3i src_offset, v3i dimensions);
	void (*texture_barrier)(struct texture* texture, u32 state);

	/* Shader. */
	struct shader* (*new_shader)(const u8* data, usize data_size);
	void (*free_shader)(struct shader* shader);

	/* Misc */
	u32 (*get_draw_call_count)();
	const char* (*get_api_name)();
	u32 (*query_features)();
};

extern struct video video;

void init_video(const struct video_config* config);
void deinit_video();

/* Like `vector', but for vertex data. */
struct vertex_vector {
	struct vertex_buffer* buffer;
	usize count;
	usize capacity;
	usize element_size;
};

void init_vertex_vector(struct vertex_vector* v, usize element_size, usize initial_capacity);
void deinit_vertex_vector(struct vertex_vector* v);
void vertex_vector_push(struct vertex_vector* v, void* elements, usize count);

inline static struct pipeline_config default_pipeline_config() {
	return (struct pipeline_config) {
		.line_width = 1.0f
	};
}
