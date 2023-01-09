#pragma once

#include "common.h"
#include "video.h"
#include "window.h"

cpp_compat void video_dx12_init(const struct video_config* config);
cpp_compat void video_dx12_deinit();

cpp_compat void video_dx12_begin(bool present);
cpp_compat void video_dx12_end(bool present);

cpp_compat bool is_dx12_supported();

cpp_compat void video_dx12_want_recreate();

cpp_compat struct framebuffer* video_dx12_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count);
cpp_compat void video_dx12_free_framebuffer(struct framebuffer* fb);
cpp_compat v2i video_dx12_get_framebuffer_size(const struct framebuffer* fb);
cpp_compat void video_dx12_resize_framebuffer(struct framebuffer* fb, v2i new_size);
cpp_compat void video_dx12_begin_framebuffer(struct framebuffer* fb);
cpp_compat void video_dx12_end_framebuffer(struct framebuffer* fb);
cpp_compat struct framebuffer* video_dx12_get_default_fb();
cpp_compat struct texture* video_dx12_get_attachment(struct framebuffer* fb, u32 index);

cpp_compat struct pipeline* video_dx12_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets);
cpp_compat struct pipeline* video_dx12_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config);
cpp_compat struct pipeline* video_dx12_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets);
cpp_compat void video_dx12_free_pipeline(struct pipeline* pipeline);
cpp_compat void video_dx12_begin_pipeline(const struct pipeline* pipeline);
cpp_compat void video_dx12_end_pipeline(const struct pipeline* pipeline);
cpp_compat void video_dx12_recreate_pipeline(struct pipeline* pipeline);
cpp_compat void video_dx12_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
cpp_compat void video_dx12_init_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data);
cpp_compat void video_dx12_pipeline_push_buffer(struct pipeline* pipeline, usize offset, usize size, const void* data);
cpp_compat void video_dx12_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target);

cpp_compat struct storage* video_dx12_new_storage(u32 flags, usize size, void* initial_data);
cpp_compat void video_dx12_update_storage(struct storage* storage, void* data);
cpp_compat void video_dx12_update_storage_region(struct storage* storage, void* data, usize offset, usize size);
cpp_compat void video_dx12_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size);
cpp_compat void video_dx12_storage_barrier(struct storage* storage, u32 state);
cpp_compat void video_dx12_storage_bind_as(const struct storage* storage, u32 as, u32 point);
cpp_compat void video_dx12_free_storage(struct storage* storage);
cpp_compat void video_dx12_invoke_compute(v3u group_count);
cpp_compat void video_dx12_register_resources();

cpp_compat struct vertex_buffer* video_dx12_new_vertex_buffer(const void* verts, usize size, u32 flags);
cpp_compat void video_dx12_free_vertex_buffer(struct vertex_buffer* vb);
cpp_compat void video_dx12_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point);
cpp_compat void video_dx12_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset);
cpp_compat void video_dx12_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size);

cpp_compat struct index_buffer* video_dx12_new_index_buffer(const void* elements, usize count, u32 flags);
cpp_compat void video_dx12_free_index_buffer(struct index_buffer* ib);
cpp_compat void video_dx12_bind_index_buffer(const struct index_buffer*);

cpp_compat void video_dx12_draw(usize count, usize offset, usize instances);
cpp_compat void video_dx12_draw_indexed(usize count, usize offset, usize instances);
cpp_compat void video_dx12_set_scissor(v4i rect);

cpp_compat struct texture* video_dx12_new_texture(const struct image* image, u32 flags, u32 format);
cpp_compat struct texture* video_dx12_new_texture_3d(v3i size, u32 flags, u32 format);
cpp_compat void video_dx12_free_texture(struct texture* texture);
cpp_compat v2i  video_dx12_get_texture_size(const struct texture* texture);
cpp_compat v3i  video_dx12_get_texture_3d_size(const struct texture* texture);
cpp_compat void video_dx12_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions);
cpp_compat void video_dx12_texture_copy_3d(struct texture* dst, v3i dst_offset, const struct texture* src, v3i src_offset, v3i dimensions);
cpp_compat void video_dx12_texture_barrier(struct texture* texture, u32 state);

cpp_compat struct shader* video_dx12_new_shader(const u8* data, usize data_size);
cpp_compat void video_dx12_free_shader(struct shader* shader);

cpp_compat m4f video_dx12_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
cpp_compat m4f video_dx12_persp(f32 fov, f32 aspect, f32 near, f32 far);

cpp_compat u32 video_dx12_get_draw_call_count();

cpp_compat u32 video_dx12_query_features();
