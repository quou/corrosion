#define GLAD_GLES2_IMPLEMENTATION
#include "gles2.h"

#include "bir.h"
#include "video_gl.h"
#include "video_internal.h"
#include "window_internal.h"

#define check_gl(x) \
	clear_gl_errors(); \
	x; \
	gl_log(__LINE__, __FILE__)

static void clear_gl_errors() {
	while (glGetError() != GL_NO_ERROR);
}

static void gl_log(u32 line, const char* file) {
	GLenum ecode;
	while ((ecode = glGetError()) != GL_NO_ERROR) {
		const char* name = "<unkown error>";

		switch (ecode) {
			case GL_INVALID_ENUM:
				name = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				name = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				name = "GL_INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				name = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				name = "GL_OUT_OF_MEMORY";
				break;
			case GL_STACK_OVERFLOW:
				name = "GL_STACK_OVERFLOW";
				break;
			case GL_STACK_UNDERFLOW:
				name = "GL_STACK_UNDERFLOW";
				break;
		}

		error("from %s:%u: glError errored with %s", file, line, name);
	}
}

struct gl_video_context gctx;

void video_gl_init(const struct video_config* config) {
	memset(&gctx, 0, sizeof gctx);

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
	check_gl(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
	check_gl(glClear(GL_COLOR_BUFFER_BIT));
}

void video_gl_end() {
	window_gl_swap();
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
	if (fb != gctx.default_fb) {
		abort_with("Not implemented");
	}
}

void video_gl_begin_framebuffer(struct framebuffer* fb) {
	if (fb == gctx.default_fb) {
		v2i window_size = get_window_size();

		check_gl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		check_gl(glViewport(0, 0, (GLsizei)window_size.x, (GLsizei)window_size.y));
	} else {
		abort_with("Not implemented");
	}
}

void video_gl_end_framebuffer(struct framebuffer* fb) {
	check_gl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

struct framebuffer* video_gl_get_default_fb() {
	return gctx.default_fb;
}

struct pipeline* video_gl_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {
	
	struct video_gl_pipeline* pipeline = core_calloc(1, sizeof *pipeline);

	pipeline->flags = flags;

	pipeline->shader = (const struct video_gl_shader*)shader;

	for (usize i = 0; i < attrib_bindings.count; i++) {
		struct pipeline_attribute_binding dst = { 0 };
		const struct pipeline_attribute_binding* src = attrib_bindings.bindings + i;

		dst.rate    = src->rate;
		dst.stride  = src->stride;
		dst.binding = src->binding;
		dst.attributes.count = src->attributes.count;
		dst.attributes.attributes = core_calloc(dst.attributes.count, sizeof(struct pipeline_attribute));

		for (usize j = 0; j < dst.attributes.count; j++) {
			struct pipeline_attribute* attrib = (void*)(dst.attributes.attributes + j);
			const struct pipeline_attribute* other = src->attributes.attributes + j;

			attrib->name     = null;
			attrib->location = other->location;
			attrib->offset   = other->offset;
			attrib->type     = other->type;
		}

		table_set(pipeline->attribute_bindings, dst.binding, dst);
	}

	if (flags & pipeline_flags_depth_test) {
		vector_push(pipeline->to_enable, GL_DEPTH_TEST);
	}

	if (flags & pipeline_flags_cull_back_face || flags & pipeline_flags_cull_front_face) {
		vector_push(pipeline->to_enable, GL_CULL_FACE);
	}

	if (flags & pipeline_flags_blend) {
		vector_push(pipeline->to_enable, GL_BLEND);
	}

	if (flags & pipeline_flags_dynamic_scissor) {
		vector_push(pipeline->to_enable, GL_SCISSOR_TEST);
	}

	if (flags & pipeline_flags_draw_lines) {
		pipeline->mode = GL_LINES;
	} else if (flags & pipeline_flags_draw_line_strip) {
		pipeline->mode = GL_LINE_STRIP;
	} else {
		pipeline->mode = GL_TRIANGLES;
	}

	check_gl(glGenVertexArrays(1, &pipeline->vao));

	return (struct pipeline*)pipeline;
}

void video_gl_free_pipeline(struct pipeline* pipeline_) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	check_gl(glDeleteVertexArrays(1, &pipeline->vao));

	free_vector(pipeline->to_enable);

	for (u32* i = table_first(pipeline->attribute_bindings); i; i = table_next(pipeline->attribute_bindings, *i)) {
		struct pipeline_attribute_binding* ab = table_get(pipeline->attribute_bindings, *i);

		core_free(ab->attributes.attributes);
	}

	free_table(pipeline->attribute_bindings);

	core_free(pipeline);
}

void video_gl_begin_pipeline(const struct pipeline* pipeline_) {
	const struct video_gl_pipeline* pipeline = (const struct video_gl_pipeline*)pipeline_;

	gctx.bound_pipeline = (void*)pipeline;

	check_gl(glBindVertexArray(pipeline->vao));

	for (usize i = 0; i < vector_count(pipeline->to_enable); i++) {
		check_gl(glEnable(pipeline->to_enable[i]));
	}

	if (pipeline->flags & pipeline_flags_cull_back_face) {
		check_gl(glCullFace(GL_BACK));
	}

	if (pipeline->flags & pipeline_flags_cull_front_face) {
		check_gl(glCullFace(GL_FRONT));
	}

	if (pipeline->flags & pipeline_flags_blend) {
		check_gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	}

	check_gl(glUseProgram(pipeline->shader->program));
}

void video_gl_end_pipeline(const struct pipeline* pipeline_) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	gctx.bound_pipeline = null;

	for (u32* i = table_first(pipeline->attribute_bindings); i; i = table_next(pipeline->attribute_bindings, *i)) {
		struct pipeline_attribute_binding* ab = table_get(pipeline->attribute_bindings, *i);

		for (usize j = 0; j < ab->attributes.count; j++) {
			struct pipeline_attribute* attr = &ab->attributes.attributes[j];

			check_gl(glDisableVertexAttribArray(attr->location));
		}
	}

	for (usize i = 0; i < vector_count(pipeline->to_enable); i++) {
		check_gl(glDisable(pipeline->to_enable[i]));
	}
}

void video_gl_recreate_pipeline(struct pipeline* pipeline) {

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

static void pipeline_setup_va(struct video_gl_pipeline* pipeline, u32 binding) {
	struct pipeline_attribute_binding* ab = table_get(pipeline->attribute_bindings, binding);

	/* A vertex buffer must be bound to call this function. */

	u32 divisor = ab->rate == pipeline_attribute_rate_per_instance ? 1 : 0;

	for (usize i = 0; i < ab->attributes.count; i++) {
		struct pipeline_attribute* attr = &ab->attributes.attributes[i];

		i32 size = 4;
		u32 type = GL_FLOAT;
		switch (attr->type) {
			case pipeline_attribute_float:
				size = 1;
				break;
			case pipeline_attribute_vec2:
				size = 2;
				break;
			case pipeline_attribute_vec3:
				size = 3;
				break;
			case pipeline_attribute_vec4:
				size = 4;
				break;
			default: break;
		}

		check_gl(glEnableVertexAttribArray(attr->location));
		check_gl(glVertexAttribPointer(attr->location, size, GL_FLOAT, GL_FALSE, ab->stride, (void*)attr->offset));
		check_gl(glVertexAttribDivisor(attr->location, divisor));
	}
}

void video_gl_pipeline_change_shader(struct pipeline* pipeline_, const struct shader* shader) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	pipeline->shader = (const struct video_gl_shader*)shader;
}

struct vertex_buffer* video_gl_new_vertex_buffer(void* verts, usize size, u32 flags) {
	struct video_gl_vertex_buffer* vb = core_calloc(1, sizeof *vb);

	vb->flags = flags;

	check_gl(glGenBuffers(1, &vb->id));
	check_gl(glBindBuffer(GL_ARRAY_BUFFER, vb->id));
	check_gl(glBufferData(GL_ARRAY_BUFFER, size, verts, flags & vertex_buffer_flags_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

	return (struct vertex_buffer*)vb;
}

void video_gl_free_vertex_buffer(struct vertex_buffer* vb_) {
	struct video_gl_vertex_buffer* vb = (struct video_gl_vertex_buffer*)vb_;

	check_gl(glDeleteBuffers(1, &vb->id));
	core_free(vb);
}

void video_gl_bind_vertex_buffer(const struct vertex_buffer* vb_, u32 point) {
	const struct video_gl_vertex_buffer* vb = (struct video_gl_vertex_buffer*)vb_;

	gctx.bound_vb = vb;

	check_gl(glBindBuffer(GL_ARRAY_BUFFER, vb->id));

	pipeline_setup_va(gctx.bound_pipeline, point);
}

void video_gl_update_vertex_buffer(struct vertex_buffer* vb, const void* data, usize size, usize offset) {
	abort_with("Not implemented");
}

void video_gl_copy_vertex_buffer(struct vertex_buffer* dst, usize dst_offset, const struct vertex_buffer* src, usize src_offset, usize size) {
	abort_with("Not implemented");
}

struct index_buffer* video_gl_new_index_buffer(void* elements, usize count, u32 flags) {
	struct video_gl_index_buffer* ib = core_calloc(1, sizeof *ib);

	ib->flags = flags;

	usize el_size = ib->flags & index_buffer_flags_u32 ? sizeof(u32) : sizeof(u16);

	check_gl(glGenBuffers(1, &ib->id));
	check_gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id));
	check_gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * el_size, elements, GL_STATIC_DRAW));

	return (struct index_buffer*)ib;
}

void video_gl_free_index_buffer(struct index_buffer* ib_) {
	struct video_gl_index_buffer* ib = (struct video_gl_index_buffer*)ib_;

	check_gl(glDeleteBuffers(1, &ib->id));

	core_free(ib);
}

void video_gl_bind_index_buffer(const struct index_buffer* ib_) {
	const struct video_gl_index_buffer* ib = (const struct video_gl_index_buffer*)ib_;

	check_gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id));

	gctx.bound_ib = ib;
}

void video_gl_draw(usize count, usize offset, usize instances) {
	check_gl(glDrawArraysInstanced(gctx.bound_pipeline->mode, offset, count, instances));
}

void video_gl_draw_indexed(usize count, usize offset, usize instances) {
	check_gl(glDrawElementsInstanced(gctx.bound_pipeline->mode, count,
		gctx.bound_vb->flags & index_buffer_flags_u32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
		(void*)offset, instances));
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

static u32 init_shader_section(u32 section, const char* src) {
	check_gl(u32 shader = glCreateShader(section));
	check_gl(glShaderSource(shader, 1, &src, null));
	check_gl(glCompileShader(shader));

	i32 success;

	check_gl(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
	if (!success) {
		char info_log[1024];
		check_gl(glGetShaderInfoLog(shader, sizeof info_log, null, info_log));
		error("Failed to compile shader: %s", info_log);
	}

	return shader;
}

static void init_shader(struct video_gl_shader* shader, const char* vert_src, const char* frag_src) {
	u32 vert = init_shader_section(GL_VERTEX_SHADER, vert_src);
	u32 frag = init_shader_section(GL_FRAGMENT_SHADER, frag_src);

	check_gl(shader->program = glCreateProgram());
	check_gl(glAttachShader(shader->program, vert));
	check_gl(glAttachShader(shader->program, frag));
	check_gl(glLinkProgram(shader->program));

	i32 success;

	check_gl(glGetProgramiv(shader->program, GL_LINK_STATUS, &success));
	if (!success) {
		char info_log[1024];
		check_gl(glGetProgramInfoLog(shader->program, sizeof info_log, null, info_log));
		error("Failed to link shader: %s", info_log);
	}

	check_gl(glDeleteShader(vert));
	check_gl(glDeleteShader(frag));
}

static void deinit_shader(struct video_gl_shader* shader) {
	check_gl(glDeleteProgram(shader->program));
}

struct shader* video_gl_new_shader(const u8* data, usize data_size) {
	struct video_gl_shader* shader = core_calloc(1, sizeof *shader);

	struct shader_header* header = (struct shader_header*)data;

	if (memcmp("CS", header->header, 2) != 0) {
		error("Invalid shader data.");
		return null;
	}

	init_shader(shader, data + header->v_gl_offset, data + header->f_gl_offset);

	return (struct shader*)shader;
}

void video_gl_free_shader(struct shader* shader) {
	deinit_shader((struct video_gl_shader*)shader);
	core_free(shader);
}

static void shader_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct shader_header* header = (struct shader_header*)raw;

	memset(payload, 0, payload_size);

	if (memcmp("CS", header->header, 2) != 0) {
		error("%s is not a valid shader resource.", filename);
		return;
	}

	init_shader(payload, raw + header->v_gl_offset, raw + header->f_gl_offset);
}

static void shader_on_unload(void* payload, usize payload_size) {
	deinit_shader(payload);
}

void video_gl_register_resources() {
	reg_res_type("shader", &(struct res_config) {
		.payload_size = sizeof(struct video_vk_shader),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_csh,
		.alt_raw_size = bir_error_csh_size,
		.on_load = shader_on_load,
		.on_unload = shader_on_unload
	});
}

u32 video_gl_get_draw_call_count() {
	return gctx.draw_call_count;
}
