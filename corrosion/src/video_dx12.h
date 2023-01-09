#pragma once

#include "common.h"
#include "video.h"
#include "window.h"

void video_dx12_init(const struct video_config* config);
void video_dx12_deinit();

void video_dx12_begin(bool present);
void video_dx12_end(bool present);

bool is_dx12_supported();

void video_dx12_want_recreate();

struct framebuffer* video_dx12_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
void video_dx12_free_framebuffer(struct framebuffer* fb);
v2i video_dx12_get_framebuffer_size(const struct framebuffer* fb);
void video_dx12_resize_framebuffer(struct framebuffer* fb, v2i new_size);
void video_dx12_begin_framebuffer(struct framebuffer* fb);
void video_dx12_end_framebuffer(struct framebuffer* fb);
struct framebuffer* video_dx12_get_default_fb();
struct texture* video_dx12_get_attachment(struct framebuffer* fb, u32 index);

struct pipeline* video_dx12_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets);
struct pipeline* video_dx12_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config);
struct pipeline* video_dx12_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets);
void video_dx12_free_pipeline(struct pipeline* pipeline);
void video_dx12_begin_pipeline(const struct pipeline* pipeline);
void video_dx12_end_pipeline(const struct pipeline* pipeline);
void video_dx12_recreate_pipeline(struct pipeline* pipeline);
void video_dx12_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
void video_dx12_init_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
void video_dx12_pipeline_push_buffer(struct pipeline* pipeline, usize offset, usize size, const void* data);
void video_dx12_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target);

struct storage* video_dx12_new_storage(u32 flags, usize size, void* initial_data);
void video_dx12_update_storage(struct storage* storage, void* data);
void video_dx12_update_storage_region(struct storage* storage, void* data, usize offset, usize size);
void video_dx12_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size);
void video_dx12_storage_barrier(struct storage* storage, u32 state);
void video_dx12_storage_bind_as(const struct storage* storage, u32 as, u32 point);
void video_dx12_free_storage(struct storage* storage);
void video_dx12_invoke_compute(v3u group_count);

void video_dx12_register_resources();

struct vertex_buffer* video_dx12_new_vertex_buffer(const void* verts, usize size, u32 flags);
void video_dx12_free_vertex_buffer(struct vertex_buffer* vb);
void video_dx12_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point);
void video_dx12_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset);
void video_dx12_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size);

struct index_buffer* video_dx12_new_index_buffer(const void* elements, usize count, u32 flags);
void video_dx12_free_index_buffer(struct index_buffer* ib);
void video_dx12_bind_index_buffer(const struct index_buffer*);

void video_dx12_draw(usize count, usize offset, usize instances);
void video_dx12_draw_indexed(usize count, usize offset, usize instances);
void video_dx12_set_scissor(v4i rect);

struct texture* video_dx12_new_texture(const struct image* image, u32 flags, u32 format);
struct texture* video_dx12_new_texture_3d(v3i size, u32 flags, u32 format);
void video_dx12_free_texture(struct texture* texture);
v2i  video_dx12_get_texture_size(const struct texture* texture);
v3i  video_dx12_get_texture_3d_size(const struct texture* texture);
void video_dx12_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);
void video_dx12_texture_copy_3d(struct texture* dst, v3i dst_offset, const struct texture* src, v3i src_offset, v3i dimensions);
void video_dx12_texture_barrier(struct texture* texture, u32 state);

struct shader* video_dx12_new_shader(const u8* data, usize data_size);
void video_dx12_free_shader(struct shader* shader);

m4f video_dx12_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
m4f video_dx12_persp(f32 fov, f32 aspect, f32 near, f32 far);

u32 video_dx12_get_draw_call_count();

u32 video_dx12_query_features();
