#ifndef cr_no_dx12

#pragma once

#include "video_dx12.h"

void video_dx12_init(const struct video_config* config) {
	abort_with("Not implemented.");
}

void video_dx12_deinit() {
	abort_with("Not implemented.");
}

void video_dx12_begin(bool present) {
	abort_with("Not implemented.");
}

void video_dx12_end(bool present) {
	abort_with("Not implemented.");
}

bool is_dx12_supported() {
	/* TODO. */
	return true;
}

void video_dx12_want_recreate() {
	abort_with("Not implemented.");
}

struct framebuffer* video_dx12_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {
	abort_with("Not implemented.");
	return null;
}

void video_dx12_free_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented.");
}

v2i video_dx12_get_framebuffer_size(const struct framebuffer* fb) {
	abort_with("Not implemented.");
}

void video_dx12_resize_framebuffer(struct framebuffer* fb, v2i new_size) {
	abort_with("Not implemented.");
}

void video_dx12_begin_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented.");
}

void video_dx12_end_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented.");
}

struct framebuffer* video_dx12_get_default_fb() {
	abort_with("Not implemented.");
}

struct texture* video_dx12_get_attachment(struct framebuffer* fb, u32 index) {
	abort_with("Not implemented.");
}

struct pipeline* video_dx12_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {
	abort_with("Not implemented.");
	return null;
}

struct pipeline* video_dx12_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config) {
	abort_with("Not implemented.");
	return null;
}

struct pipeline* video_dx12_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets) {
	abort_with("Not implemented.");
	return null;
}

void video_dx12_free_pipeline(struct pipeline* pipeline) {
	abort_with("Not implemented.");
}

void video_dx12_begin_pipeline(const struct pipeline* pipeline) {
	abort_with("Not implemented.");
}

void video_dx12_end_pipeline(const struct pipeline* pipeline) {
	abort_with("Not implemented.");
}

void video_dx12_recreate_pipeline(struct pipeline* pipeline) {
	abort_with("Not implemented.");
}

void video_dx12_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data) {
	abort_with("Not implemented.");
}

void video_dx12_init_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data) {
	abort_with("Not implemented.");
}

void video_dx12_pipeline_push_buffer(struct pipeline* pipeline, usize offset, usize size, const void* data) {
	abort_with("Not implemented.");
}

void video_dx12_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target) {
	abort_with("Not implemented.");
}

struct storage* video_dx12_new_storage(u32 flags, usize size, void* initial_data) {
	abort_with("Not implemented.");
	return null;
}

void video_dx12_update_storage(struct storage* storage, void* data) {
	abort_with("Not implemented.");
}

void video_dx12_update_storage_region(struct storage* storage, void* data, usize offset, usize size) {
	abort_with("Not implemented.");
}

void video_dx12_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size) {
	abort_with("Not implemented.");
}

void video_dx12_storage_barrier(struct storage* storage, u32 state) {
	abort_with("Not implemented.");
}

void video_dx12_storage_bind_as(const struct storage* storage, u32 as, u32 point) {
	abort_with("Not implemented.");
}

void video_dx12_free_storage(struct storage* storage) {
	abort_with("Not implemented.");
}

void video_dx12_invoke_compute(v3u group_count) {
	abort_with("Not implemented.");
}

void video_dx12_register_resources() {
	abort_with("Not implemented.");
}

struct vertex_buffer* video_dx12_new_vertex_buffer(const void* verts, usize size, u32 flags) {
	abort_with("Not implemented.");
}

void video_dx12_free_vertex_buffer(struct vertex_buffer* vb) {
	abort_with("Not implemented.");
}

void video_dx12_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point) {
	abort_with("Not implemented.");
}

void video_dx12_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset) {
	abort_with("Not implemented.");
}

void video_dx12_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size) {
	abort_with("Not implemented.");
}

struct index_buffer* video_dx12_new_index_buffer(const void* elements, usize count, u32 flags) {
	abort_with("Not implemented.");
	return null;
}

void video_dx12_free_index_buffer(struct index_buffer* ib) {
	abort_with("Not implemented.");
}

void video_dx12_bind_index_buffer(const struct index_buffer* ib) {
	abort_with("Not implemented.");
}

void video_dx12_draw(usize count, usize offset, usize instances) {
	abort_with("Not implemented.");
}

void video_dx12_draw_indexed(usize count, usize offset, usize instances) {
	abort_with("Not implemented.");
}

void video_dx12_set_scissor(v4i rect) {
	abort_with("Not implemented.");
}

struct texture* video_dx12_new_texture(const struct image* image, u32 flags, u32 format) {
	abort_with("Not implemented.");
	return null;
}

struct texture* video_dx12_new_texture_3d(v3i size, u32 flags, u32 format) {
	abort_with("Not implemented.");
}

void video_dx12_free_texture(struct texture* texture) {
	abort_with("Not implemented.");
}

v2i  video_dx12_get_texture_size(const struct texture* texture)  {
	abort_with("Not implemented.");
	return v2i_zero();
}

v3i  video_dx12_get_texture_3d_size(const struct texture* texture)  {
	abort_with("Not implemented.");
	return v3i_zero();
}

void video_dx12_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions)  {
	abort_with("Not implemented.");
}

void video_dx12_texture_copy_3d(struct texture* dst, v3i dst_offset, const struct texture* src, v3i src_offset, v3i dimensions)  {
	abort_with("Not implemented.");
}

void video_dx12_texture_barrier(struct texture* texture, u32 state)  {
	abort_with("Not implemented.");
}

struct shader* video_dx12_new_shader(const u8* data, usize data_size)  {
	abort_with("Not implemented.");
}

void video_dx12_free_shader(struct shader* shader)  {
	abort_with("Not implemented.");
}

m4f video_dx12_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)  {
	abort_with("Not implemented.");
}

m4f video_dx12_persp(f32 fov, f32 aspect, f32 near, f32 far)  {
	abort_with("Not implemented.");
}

u32 video_dx12_get_draw_call_count()  {
	abort_with("Not implemented.");
}

u32 video_dx12_query_features()  {
	abort_with("Not implemented.");
}

#endif