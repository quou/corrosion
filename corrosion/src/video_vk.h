#pragma once

#include "common.h"
#include "video.h"
#include "window.h"

void video_vk_init(const struct video_config* config);
void video_vk_deinit();

void video_vk_begin(bool present);
void video_vk_end(bool present);

void video_vk_want_recreate();

struct framebuffer* video_vk_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
void video_vk_free_framebuffer(struct framebuffer* fb);
v2i video_vk_get_framebuffer_size(const struct framebuffer* fb);
void video_vk_resize_framebuffer(struct framebuffer* fb, v2i new_size);
void video_vk_begin_framebuffer(struct framebuffer* fb);
void video_vk_end_framebuffer(struct framebuffer* fb);
struct framebuffer* video_vk_get_default_fb();
struct texture* video_vk_get_attachment(struct framebuffer* fb, u32 index);

struct pipeline* video_vk_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets);
struct pipeline* video_vk_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config);
struct pipeline* video_vk_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets);
void video_vk_free_pipeline(struct pipeline* pipeline);
void video_vk_begin_pipeline(const struct pipeline* pipeline);
void video_vk_end_pipeline(const struct pipeline* pipeline);
void video_vk_recreate_pipeline(struct pipeline* pipeline);
void video_vk_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
void video_vk_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target);

struct storage* video_vk_new_storage(u32 flags, usize size, void* initial_data);
void video_vk_update_storage(struct storage* storage, void* data);
void video_vk_update_storage_region(struct storage* storage, void* data, usize offset, usize size);
void video_vk_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size);
void video_vk_storage_barrier(struct storage* storage, u32 state);
void video_vk_storage_bind_as(const struct storage* storage, u32 as, u32 point);
void video_vk_free_storage(struct storage* storage);
void video_vk_invoke_compute(v3u group_count);

void video_vk_register_resources();

struct vertex_buffer* video_vk_new_vertex_buffer(void* verts, usize size, u32 flags);
void video_vk_free_vertex_buffer(struct vertex_buffer* vb);
void video_vk_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point);
void video_vk_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset);
void video_vk_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size);

struct index_buffer* video_vk_new_index_buffer(void* elements, usize count, u32 flags);
void video_vk_free_index_buffer(struct index_buffer* ib);
void video_vk_bind_index_buffer(const struct index_buffer* );

void video_vk_draw(usize count, usize offset, usize instances);
void video_vk_draw_indexed(usize count, usize offset, usize instances);
void video_vk_set_scissor(v4i rect);

struct texture* video_vk_new_texture(const struct image* image, u32 flags, u32 format);
struct texture* video_vk_new_texture_3d(v3i size, u32 flags, u32 format);
void video_vk_free_texture(struct texture* texture);
v2i  video_vk_get_texture_size(const struct texture* texture);
v3i  video_vk_get_texture_3d_size(const struct texture* texture);
void video_vk_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);
void video_vk_texture_copy_3d(struct texture* dst, v3i dst_offset, const struct texture* src, v3i src_offset, v3i dimensions);
void video_vk_texture_barrier(struct texture* texture, u32 state);

struct shader* video_vk_new_shader(const u8* data, usize data_size);
void video_vk_free_shader(struct shader* shader);

m4f video_vk_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
m4f video_vk_persp(f32 fov, f32 aspect, f32 near, f32 far);

u32 video_vk_get_draw_call_count();

u32 video_vk_query_features();
