#pragma once

#include "common.h"
#include "video.h"
#include "window.h"

void video_gl_init(const struct video_config* config);
void video_gl_deinit();

void video_gl_begin();
void video_gl_end();

void video_gl_want_recreate();

struct framebuffer* video_gl_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
void video_gl_free_framebuffer(struct framebuffer* fb);
v2i video_gl_get_framebuffer_size(const struct framebuffer* fb);
void video_gl_resize_framebuffer(struct framebuffer* fb, v2i new_size);
void video_gl_begin_framebuffer(struct framebuffer* fb);
void video_gl_end_framebuffer(struct framebuffer* fb);
struct framebuffer* video_gl_get_default_fb();
struct texture* video_gl_get_attachment(struct framebuffer* fb, u32 index);

struct pipeline* video_gl_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets);
struct pipeline* video_gl_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config);
struct pipeline* video_gl_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets);
void video_gl_free_pipeline(struct pipeline* pipeline);
void video_gl_begin_pipeline(const struct pipeline* pipeline);
void video_gl_end_pipeline(const struct pipeline* pipeline);
void video_gl_recreate_pipeline(struct pipeline* pipeline);
void video_gl_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
void video_gl_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target);
void video_gl_pipeline_add_descriptor_set(struct pipeline* pipeline, const struct pipeline_descriptor_set* set);
void video_gl_pipeline_change_shader(struct pipeline* pipeline, const struct shader* shader);

struct storage* video_gl_new_storage(u32 flags, usize size, void* initial_data);
void video_gl_update_storage(struct storage* storage, u32 mode, void* data);
void video_gl_update_storage_region(struct storage* storage, u32 mode, void* data, usize offset, usize size);
void video_gl_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size);
void video_gl_storage_barrier(struct storage* storage, u32 state);
void video_gl_storage_bind_as(const struct storage* storage, u32 as, u32 point);
void video_gl_free_storage(struct storage* storage);
void video_gl_invoke_compute(v3u count);

void video_gl_register_resources();

struct vertex_buffer* video_gl_new_vertex_buffer(void* verts, usize size, u32 flags);
void video_gl_free_vertex_buffer(struct vertex_buffer* vb);
void video_gl_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point);
void video_gl_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset);
void video_gl_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size);

struct index_buffer* video_gl_new_index_buffer(void* elements, usize count, u32 flags);
void video_gl_free_index_buffer(struct index_buffer* ib);
void video_gl_bind_index_buffer(const struct index_buffer* );

void video_gl_draw(usize count, usize offset, usize instances);
void video_gl_draw_indexed(usize count, usize offset, usize instances);
void video_gl_set_scissor(v4i rect);

struct texture* video_gl_new_texture(const struct image* image, u32 flags);
void video_gl_free_texture(struct texture* texture);
v2i  video_gl_get_texture_size(const struct texture* texture);
void video_gl_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);

struct shader* video_gl_new_shader(const u8* data, usize data_size);
void video_gl_free_shader(struct shader* shader);

u32 video_gl_get_draw_call_count();

