#define GLAD_GLES2_IMPLEMENTATION
#include "gles2.h"

#include "video_gl.h"
#include "window_internal.h"

void video_gl_init(const struct video_config* config) {
	window_create_gl_context();

	if (!gladLoadGLES2((GLADloadfunc)window_get_gl_proc)) {
		abort_with("Failed to load OpenGL ES functions.");
	}

	info("GL_VENDOR: \t%s.", glGetString(GL_VENDOR));
	info("GL_RENDERER: \t%s.", glGetString(GL_RENDERER));
	info("GL_VERSION: \t%s.", glGetString(GL_VERSION));
}

void video_gl_deinit() {
	window_destroy_gl_context();
}

void video_gl_begin() {
	abort_with("Not implemented");
}

void video_gl_end() {
	abort_with("Not implemented");
}

void video_gl_want_recreate() {
	abort_with("Not implemented");
}

struct framebuffer* video_gl_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented");
}

v2i video_gl_get_framebuffer_size(const struct framebuffer* fb) {
	abort_with("Not implemented");
	return v2i_zero();
}

void video_gl_resize_framebuffer(struct framebuffer* fb, v2i new_size) {
	abort_with("Not implemented");
}

void video_gl_begin_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented");
}

void video_gl_end_framebuffer(struct framebuffer* fb) {
	abort_with("Not implemented");
}

struct framebuffer* video_gl_get_default_fb() {
	abort_with("Not implemented");
	return null;
}

struct pipeline* video_gl_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_pipeline(struct pipeline* pipeline) {
	abort_with("Not implemented");
}

void video_gl_begin_pipeline(const struct pipeline* pipeline) {
	abort_with("Not implemented");
}

void video_gl_end_pipeline(const struct pipeline* pipeline) {
	abort_with("Not implemented");
}

void video_gl_recreate_pipeline(struct pipeline* pipeline) {
	abort_with("Not implemented");
}

void video_gl_update_pipeline_uniform(struct pipeline* pipeline, const char* set, const char* descriptor, const void* data) {
	abort_with("Not implemented");
}

void video_gl_bind_pipeline_descriptor_set(struct pipeline* pipeline, const char* set, usize target) {
	abort_with("Not implemented");
}

void video_gl_pipeline_add_descriptor_set(struct pipeline* pipeline, const struct pipeline_descriptor_set* set) {
	abort_with("Not implemented");
}

void video_gl_pipeline_change_shader(struct pipeline* pipeline, const struct shader* shader) {
	abort_with("Not implemented");
}

void video_gl_register_resources() {
	abort_with("Not implemented");
}

struct vertex_buffer* video_gl_new_vertex_buffer(void* verts, usize size, u32 flags) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_vertex_buffer(struct vertex_buffer* vb) {
	abort_with("Not implemented");
}

void video_gl_bind_vertex_buffer(const struct vertex_buffer* vb, u32 point) {
	abort_with("Not implemented");
}

void video_gl_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset) {
	abort_with("Not implemented");
}

void video_gl_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size) {
	abort_with("Not implemented");
}

struct index_buffer* video_gl_new_index_buffer(void* elements, usize count, u32 flags) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_index_buffer(struct index_buffer* ib) {
	abort_with("Not implemented");
}

void video_gl_bind_index_buffer(const struct index_buffer* ib) {
	abort_with("Not implemented");
}

void video_gl_draw(usize count, usize offset, usize instances) {
	abort_with("Not implemented");
}

void video_gl_draw_indexed(usize count, usize offset, usize instances) {
	abort_with("Not implemented");
}

void video_gl_set_scissor(v4i rect) {
	abort_with("Not implemented");
}

struct texture* video_gl_new_texture(const struct image* image, u32 flags) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_texture(struct texture* texture) {
	abort_with("Not implemented");
}

v2i  video_gl_get_texture_size(const struct texture* texture) {
	abort_with("Not implemented");
	return v2i_zero();
}

void video_gl_texture_copy(struct texture* dst, v2i dst_offset, const struct texture* src, v2i src_offset, v2i dimensions) {
	abort_with("Not implemented");
}

struct shader* video_gl_new_shader(const u8* data, usize data_size) {
	abort_with("Not implemented");
	return null;
}

void video_gl_free_shader(struct shader* shader) {
	abort_with("Not implemented");
}

m4f video_gl_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	abort_with("Not implemented");
	return m4f_identity();
}

m4f video_gl_persp(f32 fov, f32 aspect, f32 near, f32 far) {
	abort_with("Not implemented");
	return m4f_identity();
}

u32 video_gl_get_draw_call_count() {
	abort_with("Not implemented");
	return 0;
}
