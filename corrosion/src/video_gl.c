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

		error("from %s:%u: glError returned %s", file, line, name);
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

	gctx.default_fb = (void*)-1;
	gctx.default_clear = config->clear_colour;
}

void video_gl_deinit() {
	window_destroy_gl_context();
}

void video_gl_begin() {
	if (gctx.want_recreate) {
		struct video_gl_framebuffer* framebuffer = gctx.framebuffers.head;
		while (framebuffer) {
			if (framebuffer->flags & framebuffer_flags_fit) {
				video_gl_resize_framebuffer((struct framebuffer*)framebuffer, get_window_size());
			}

			framebuffer = framebuffer->next;
		}

		struct video_gl_pipeline* pipeline = gctx.pipelines.head;
		while (pipeline) {
			video_gl_recreate_pipeline((struct pipeline*)pipeline);

			pipeline = pipeline->next;
		}

		gctx.want_recreate = false;
	}
}

void video_gl_end() {
	window_gl_swap();
}

void video_gl_want_recreate() {
	gctx.want_recreate = true;
}

static void video_gl_init_framebuffer(struct video_gl_framebuffer* fb, u32 flags, v2i size,
	const struct framebuffer_attachment_desc* attachments, usize attachment_count) {

	memset(&fb->attachment_map, 0, sizeof fb->attachment_map);

	fb->size = size;
	fb->flags = flags;
	fb->colour_count = 0;
	fb->has_depth = false;

	check_gl(glGenFramebuffers(1, &fb->id));
	check_gl(glGenFramebuffers(1, &fb->flipped_fb));
	check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->id));

	for (usize i = 0; i < attachment_count; i++) {
		const struct framebuffer_attachment_desc* desc = &attachments[i];

		if (desc->type == framebuffer_attachment_colour) {
			fb->colour_count++;
		} else if (desc->type == framebuffer_attachment_depth) {
			fb->has_depth = true;
		}
	}

	if (fb->colour_count) {
		fb->colours = core_calloc(fb->colour_count, sizeof *fb->colours);
		check_gl(glGenTextures(fb->colour_count, fb->colours));

		fb->flipped_colours = core_calloc(fb->colour_count, sizeof *fb->flipped_colours);

		fb->colour_formats = core_calloc(fb->colour_count, sizeof *fb->colour_formats);
		fb->colour_types = core_calloc(fb->colour_count, sizeof *fb->colour_types);
		fb->clear_colours = core_calloc(fb->colour_count, sizeof *fb->clear_colours);

		fb->draw_buffers = core_calloc(fb->colour_count, sizeof *fb->draw_buffers);
		fb->draw_buffers2 = core_calloc(fb->colour_count, sizeof *fb->draw_buffers2);
	}

	fb->colour_count = 0;

	/* Colour attachments. */
	for (usize i = 0; i < attachment_count; i++) {
		const struct framebuffer_attachment_desc* desc = &attachments[i];

		if (desc->type == framebuffer_attachment_colour) {
			usize colour_index = fb->colour_count++;

			check_gl(glBindTexture(GL_TEXTURE_2D, fb->colours[colour_index]));

			GLenum format = GL_RGBA;
			GLenum type = GL_UNSIGNED_BYTE;

			usize pixel_size = 4;

			switch (desc->format) {
				case framebuffer_format_rgba8i:
					format = GL_RGBA;
					type = GL_UNSIGNED_BYTE;
					pixel_size = 4;
					break;
				case framebuffer_format_rgba16f:
					format = GL_RGBA16F;
					type = GL_HALF_FLOAT;
					pixel_size = 8;
					break;
				case framebuffer_format_rgba32f:
					format = GL_RGBA32F;
					type = GL_FLOAT;
					pixel_size = 16;
					break;
			}

			check_gl(glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, GL_RGBA, type, null));

			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			check_gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colour_index, GL_TEXTURE_2D, fb->colours[colour_index], 0));

			u32 flipped;
			check_gl(glGenTextures(1, &flipped));
			check_gl(glBindTexture(GL_TEXTURE_2D, flipped));
			check_gl(glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, GL_RGBA, type, null));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->flipped_fb));
			check_gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colour_index, GL_TEXTURE_2D, flipped, 0));
			check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->id));

			fb->draw_buffers[colour_index] = GL_COLOR_ATTACHMENT0 + colour_index;

			fb->flipped_colours[colour_index] = (struct video_gl_texture) {
				.id = flipped,
				.size = fb->size
			};

			fb->colour_formats[colour_index] = format;
			fb->colour_types[colour_index] = type;

			fb->clear_colours[colour_index] = desc->clear_colour;

			table_set(fb->attachment_map, i, &fb->flipped_colours[colour_index]);
		} else if (desc->type == framebuffer_attachment_depth) {
			check_gl(glGenTextures(1, &fb->depth_attachment));
			check_gl(glBindTexture(GL_TEXTURE_2D, fb->depth_attachment));

			check_gl(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.x, size.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, null));

			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			check_gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb->depth_attachment, 0));

			usize pixel_size = 4;

			u32 flipped;
			check_gl(glGenTextures(1, &flipped));
			check_gl(glBindTexture(GL_TEXTURE_2D, flipped));
			check_gl(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.x, size.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, null));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->flipped_fb));
			check_gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, flipped, 0));
			check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->id));

			table_set(fb->attachment_map, i, &fb->flipped_depth);
		}
	}

	check_gl(u32 stat = glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (stat != GL_FRAMEBUFFER_COMPLETE) {
		abort_with("Failed to create framebuffer.");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void video_gl_deinit_framebuffer(struct video_gl_framebuffer* fb) {
	core_free(fb->colours);
	core_free(fb->flipped_colours);
	core_free(fb->colour_types);
	core_free(fb->colour_formats);
	core_free(fb->draw_buffers);
	core_free(fb->draw_buffers2);
	core_free(fb->clear_colours);
	free_table(fb->attachment_map);
}

struct framebuffer* video_gl_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {
	struct video_gl_framebuffer* fb = core_alloc(sizeof *fb);

	fb->attachment_descs = core_alloc(attachment_count * sizeof(struct framebuffer_attachment_desc));
	memcpy(fb->attachment_descs, attachments, attachment_count * sizeof(struct framebuffer_attachment_desc));
	fb->attachment_count = attachment_count;

	video_gl_init_framebuffer(fb, flags, size, attachments, attachment_count);

	list_push(gctx.framebuffers, fb);

	return (struct framebuffer*)fb;
}

void video_gl_free_framebuffer(struct framebuffer* fb_) {
	struct video_gl_framebuffer* fb = (struct video_gl_framebuffer*)fb_;

	video_gl_deinit_framebuffer(fb);

	list_remove(gctx.framebuffers, fb);

	core_free(fb->attachment_descs);
	core_free(fb);
}

v2i video_gl_get_framebuffer_size(const struct framebuffer* fb_) {
	const struct video_gl_framebuffer* fb = (const struct video_gl_framebuffer*)fb_;

	if (fb_ == gctx.default_fb) {
		return get_window_size();
	}

	return fb->size;
}

void video_gl_resize_framebuffer(struct framebuffer* fb_, v2i new_size) {
	struct video_gl_framebuffer* fb = (struct video_gl_framebuffer*)fb_;

	video_gl_deinit_framebuffer(fb);
	video_gl_init_framebuffer(fb, fb->flags, new_size, fb->attachment_descs, fb->attachment_count);
}

void video_gl_begin_framebuffer(struct framebuffer* fb_) {
	struct video_gl_framebuffer* fb = (struct video_gl_framebuffer*)fb_;

	gctx.bound_fb = fb;

	if (fb_ == gctx.default_fb) {
		v2i window_size = get_window_size();

		check_gl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		check_gl(glViewport(0, 0, (GLsizei)window_size.x, (GLsizei)window_size.y));

		v4f c = gctx.default_clear;

		check_gl(glClearColor(c.r, c.g, c.b, c.a));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->id));
		check_gl(glViewport(0, 0, fb->size.x, fb->size.y));

		check_gl(glDrawBuffers(fb->colour_count, fb->draw_buffers));

		for (usize i = 0; i < fb->colour_count; i++) {
			v4f c = fb->clear_colours[i];

			GLenum a = GL_COLOR_ATTACHMENT0 + i;

			float colour[] = { c.r, c.g, c.b, c.a };

			check_gl(glClearBufferfv(GL_COLOR, i, colour));
		}
		
		if (fb->has_depth) {
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}
}

void video_gl_end_framebuffer(struct framebuffer* fb_) {
	struct video_gl_framebuffer* fb = (struct video_gl_framebuffer*)fb_;

	if (fb_ != gctx.default_fb) {
		/* Flip the framebuffer. */
		check_gl(glBindFramebuffer(GL_FRAMEBUFFER, fb->flipped_fb));
		check_gl(glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->id));

		for (usize i = 0; i < fb->colour_count; i++) {
			u32 a = GL_COLOR_ATTACHMENT0 + i;

			for (usize j = 0; j < fb->colour_count; j++) {
				fb->draw_buffers2[j] = GL_NONE;
			}

			fb->draw_buffers2[i] = a;

			check_gl(glDrawBuffers(fb->colour_count, fb->draw_buffers2));
			check_gl(glReadBuffer(a));

			check_gl(glBlitFramebuffer(0, 0, fb->size.x, fb->size.y, 0, fb->size.y, fb->size.x, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST));
		}

		if (fb->has_depth) {
			check_gl(glBlitFramebuffer(0, 0, fb->size.x, fb->size.y, 0, fb->size.y, fb->size.x, 0, GL_DEPTH_BUFFER_BIT, GL_NEAREST));
		}
	}

	gctx.bound_fb = null;
	check_gl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

struct framebuffer* video_gl_get_default_fb() {
	return gctx.default_fb;
}

struct texture* video_gl_get_attachment(struct framebuffer* fb_, u32 index) {
	struct video_gl_framebuffer* fb = (struct video_gl_framebuffer*)fb_;
	return *(struct texture**)(table_get(fb->attachment_map, (usize)index));
	return null;
}

struct pipeline* video_gl_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {
	struct pipeline_config default_config = default_pipeline_config();

	return video_gl_new_pipeline_ex(flags, shader, framebuffer, attrib_bindings, descriptor_sets, &default_config);
}

static void init_pipeline(struct video_gl_pipeline* pipeline, u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config) {

	pipeline->flags = flags;

	pipeline->shader = (const struct video_gl_shader*)shader;
	pipeline->framebuffer = (const struct video_gl_framebuffer*)framebuffer;

	pipeline->config = *config;

	/* Copy descriptor sets. */
	for (usize i = 0; i < descriptor_sets.count; i++) {
		struct pipeline_descriptor_set* set = &descriptor_sets.sets[i];

		u64 set_key = hash_string(set->name);

		struct video_gl_descriptor_set target_set = { 0 };
		target_set.count = set->count;

		for (usize j = 0; j < set->count; j++) {
			struct pipeline_descriptor* desc = &set->descriptors[j];

			struct video_gl_descriptor target_desc = {
				.binding = desc->binding,
				.stage = desc->stage,
				.resource = desc->resource
			};

			if (desc->resource.type == pipeline_resource_uniform_buffer) {
				check_gl(glGenBuffers(1, &target_desc.ub_id));
				check_gl(glBindBuffer(GL_UNIFORM_BUFFER, target_desc.ub_id));
				check_gl(glBufferData(GL_UNIFORM_BUFFER, desc->resource.uniform.size, null, GL_DYNAMIC_DRAW));
				target_desc.ub_size = desc->resource.uniform.size;
			}

			table_set(target_set.descriptors, hash_string(desc->name), target_desc);
		}

		table_set(pipeline->descriptor_sets, set_key, target_set);
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
}

static void deinit_pipeline(struct video_gl_pipeline* pipeline) {
	check_gl(glDeleteVertexArrays(1, &pipeline->vao));

	free_vector(pipeline->to_enable);

	for (u64* i = table_first(pipeline->descriptor_sets); i; i = table_next(pipeline->descriptor_sets, *i)) {
		struct video_gl_descriptor_set* set = table_get(pipeline->descriptor_sets, *i);

		for (u64* j = table_first(set->descriptors); j; j = table_next(set->descriptors, *j)) {
			struct video_gl_descriptor* desc = table_get(set->descriptors, *j);

			if (desc->resource.type == pipeline_resource_uniform_buffer) {
				check_gl(glDeleteBuffers(1, &desc->ub_id));
			}
		}

		free_table(set->descriptors);
	}

	free_table(pipeline->descriptor_sets);
}

struct pipeline* video_gl_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config) {
	
	struct video_gl_pipeline* pipeline = core_calloc(1, sizeof *pipeline);

	/* Copy attribute bindings. */
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

	if (descriptor_sets.count > 0) {
		vector_allocate(pipeline->copy_descriptor_sets, descriptor_sets.count);

		for (usize i = 0; i < descriptor_sets.count; i++) {
			struct pipeline_descriptor_set set = { 0 };
			const struct pipeline_descriptor_set* other = descriptor_sets.sets + i;

			copy_pipeline_descriptor_set(&set, other);

			vector_push(pipeline->copy_descriptor_sets, set);
		}
	}

	if (~flags & pipeline_flags_compute) {
		pipeline->bindings.count = attrib_bindings.count;
		pipeline->bindings.bindings = core_calloc(attrib_bindings.count, sizeof *pipeline->bindings.bindings);

		for (usize i = 0; i < pipeline->bindings.count; i++) {
			struct pipeline_attribute_binding* dst = (void*)(pipeline->bindings.bindings + i);
			const struct pipeline_attribute_binding* src = attrib_bindings.bindings + i;

			dst->rate    = src->rate;
			dst->stride  = src->stride;
			dst->binding = src->binding;
			dst->attributes.count = src->attributes.count;
			dst->attributes.attributes = core_calloc(dst->attributes.count, sizeof(struct pipeline_attribute));

			for (usize j = 0; j < dst->attributes.count; j++) {
				struct pipeline_attribute* attrib = (void*)(dst->attributes.attributes + j);
				const struct pipeline_attribute* other = src->attributes.attributes + j;

				attrib->name     = null;
				attrib->location = other->location;
				attrib->offset   = other->offset;
				attrib->type     = other->type;
			}
		}
	}

	list_push(gctx.pipelines, pipeline);

	init_pipeline(pipeline, flags, shader, framebuffer, attrib_bindings, descriptor_sets, config);

	return (struct pipeline*)pipeline;
}

struct pipeline* video_gl_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets) {
	abort_with("Not implemented.");
	return null;
}

void video_gl_free_pipeline(struct pipeline* pipeline_) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	deinit_pipeline(pipeline);

	for (u32* i = table_first(pipeline->attribute_bindings); i; i = table_next(pipeline->attribute_bindings, *i)) {
		struct pipeline_attribute_binding* ab = table_get(pipeline->attribute_bindings, *i);

		core_free(ab->attributes.attributes);
	}

	for (usize i = 0; i < vector_count(pipeline->copy_descriptor_sets); i++) {
		struct pipeline_descriptor_set* set = (void*)(pipeline->copy_descriptor_sets + i);

		core_free((void*)set->name);

		for (usize ii = 0; ii < set->count; ii++) {
			core_free((void*)set->descriptors[ii].name);
		}

		free_vector(set->descriptors);
	}

	if (~pipeline->flags & pipeline_flags_compute) {
		for (usize i = 0; i < pipeline->bindings.count; i++) {
			struct pipeline_attribute_binding* binding = (void*)(pipeline->bindings.bindings + i);

			core_free(binding->attributes.attributes);
		}

		core_free(pipeline->bindings.bindings);
	}

	list_remove(gctx.pipelines, pipeline);

	free_vector(pipeline->copy_descriptor_sets);

	free_table(pipeline->attribute_bindings);

	core_free(pipeline);
}

void video_gl_begin_pipeline(const struct pipeline* pipeline_) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

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

	if (pipeline->flags & pipeline_flags_depth_test) {
		check_gl(glDepthFunc(GL_LESS));
	}

	if (pipeline->flags & pipeline_flags_dynamic_scissor) {
		v2i fb_size = video_gl_get_framebuffer_size((const struct framebuffer*)gctx.bound_fb);

		check_gl(glScissor(0, 0, fb_size.x, fb_size.y));
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

void video_gl_invoke_compute(v3u count) {
	abort_with("Not implemented.");
}

void video_gl_recreate_pipeline(struct pipeline* pipeline_) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	deinit_pipeline(pipeline);

	if (pipeline->flags & pipeline_flags_compute) {
	/*	init_compute_pipeline(pipeline, pipeline->flags,
			(const struct video_vk_shader*)pipeline->shader,
			&(struct pipeline_descriptor_sets) {
				.sets = pipeline->descriptor_sets,
				.count = vector_count(pipeline->descriptor_sets)
			});*/
	} else {
		init_pipeline(pipeline, pipeline->flags, (const struct shader*)pipeline->shader, (const struct framebuffer*)pipeline->framebuffer,
			pipeline->bindings, (struct pipeline_descriptor_sets) {
				.sets = pipeline->copy_descriptor_sets,
				.count = vector_count(pipeline->copy_descriptor_sets)
			}, &pipeline->config);
	}
}

void video_gl_update_pipeline_uniform(struct pipeline* pipeline_, const char* set, const char* descriptor, const void* data) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	u64 set_name_hash  = hash_string(set);
	u64 desc_name_hash = hash_string(descriptor);

	struct video_gl_descriptor_set* desc_set = table_get(pipeline->descriptor_sets, set_name_hash);
	if (!desc_set) {
		error("%s: No such descriptor set.", set);
		return;
	}

	struct video_gl_descriptor* desc = table_get(desc_set->descriptors, desc_name_hash);
	if (!desc) {
		error("%s: No such descriptor on descriptor set `%s'.", descriptor, set);
		return;
	}

	check_gl(glBindBuffer(GL_UNIFORM_BUFFER, desc->ub_id));
	check_gl(glBufferSubData(GL_UNIFORM_BUFFER, 0, desc->ub_size, data));
}

void video_gl_bind_pipeline_descriptor_set(struct pipeline* pipeline_, const char* set, usize target) {
	struct video_gl_pipeline* pipeline = (struct video_gl_pipeline*)pipeline_;

	struct video_gl_descriptor_set* desc_set = table_get(pipeline->descriptor_sets, hash_string(set));
	if (!desc_set) {
		error("%s: No such descriptor set.", set);
		return;
	}

	for (u64* i = table_first(desc_set->descriptors); i; i = table_next(desc_set->descriptors, *i)) {
		struct video_gl_descriptor* desc = table_get(desc_set->descriptors, *i);

		u32 binding = target * 16 + desc->binding;

		switch (desc->resource.type) {
			case pipeline_resource_texture: {
				const struct video_gl_texture* texture = (const struct video_gl_texture*)desc->resource.texture;

				check_gl(glActiveTexture(GL_TEXTURE0 + binding));
				check_gl(glBindTexture(GL_TEXTURE_2D, texture->id));
			} break;
			case pipeline_resource_uniform_buffer: {
				check_gl(glBindBufferBase(GL_UNIFORM_BUFFER, binding, desc->ub_id));
			} break;
		}
	}
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

struct storage* video_gl_new_storage(u32 flags, usize size, void* initial_data) {
	abort_with("Not implemented.");
	return null;
}

void video_gl_update_storage(struct storage* storage, void* data) {
	abort_with("Not implemented.");
}

void video_gl_update_storage_region(struct storage* storage, void* data, usize offset, usize size) {
	abort_with("Not implemented.");
}

void video_gl_free_storage(struct storage* storage) {
	abort_with("Not implemented.");
}

void video_gl_copy_storage(struct storage* dst, usize dst_offset, const struct storage* src, usize src_offset, usize size) {
	abort_with("Not implemented.");
}

void video_gl_storage_barrier(struct storage* storage, u32 state) {
	abort_with("Not implemented.");
}

void video_gl_storage_bind_as(const struct storage* storage, u32 as, u32 point) {
	abort_with("Not implemented.");
}

struct vertex_buffer* video_gl_new_vertex_buffer(void* verts, usize size, u32 flags) {
	struct video_gl_vertex_buffer* vb = core_calloc(1, sizeof *vb);

	vb->flags = flags;
	vb->mode = flags & vertex_buffer_flags_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

	check_gl(glGenBuffers(1, &vb->id));
	check_gl(glBindBuffer(GL_ARRAY_BUFFER, vb->id));
	check_gl(glBufferData(GL_ARRAY_BUFFER, size, verts, vb->mode));

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

void video_gl_update_vertex_buffer(struct vertex_buffer* vb_, const void* data, usize size, usize offset) {
	const struct video_gl_vertex_buffer* vb = (struct video_gl_vertex_buffer*)vb_;

	check_gl(glBindBuffer(GL_ARRAY_BUFFER, vb->id));
	check_gl(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void video_gl_copy_vertex_buffer(struct vertex_buffer* dst_, usize dst_offset, const struct vertex_buffer* src_, usize src_offset, usize size) {
	struct video_gl_vertex_buffer* dst = (struct video_gl_vertex_buffer*)dst_;
	const struct video_gl_vertex_buffer* src = (const struct video_gl_vertex_buffer*)src_;

	check_gl(glBindBuffer(GL_COPY_READ_BUFFER,  src->id));
	check_gl(glBindBuffer(GL_COPY_WRITE_BUFFER, dst->id));

	check_gl(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, src_offset, dst_offset, size));

	check_gl(glBindBuffer(GL_COPY_READ_BUFFER,  0));
	check_gl(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
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
	gctx.draw_call_count++;
}

void video_gl_draw_indexed(usize count, usize offset, usize instances) {
	check_gl(glDrawElementsInstanced(gctx.bound_pipeline->mode, count,
		gctx.bound_vb->flags & index_buffer_flags_u32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
		(void*)offset, instances));
	gctx.draw_call_count++;
}

void video_gl_set_scissor(v4i rect) {
	v2i fb_size = video_gl_get_framebuffer_size((const struct framebuffer*)gctx.bound_fb);

	glScissor(rect.x, fb_size.y - (rect.y + rect.w), rect.z, rect.w);
}

static void init_texture(struct video_gl_texture* texture, const struct image* image, u32 flags, u32 format) {
	texture->flags = flags;
	texture->size = image->size;

	GLenum gl_format = GL_RGBA;
	GLenum gl_type = GL_UNSIGNED_BYTE;
	switch (format) {
		case texture_format_r8i:
			gl_format = GL_RED;
			gl_type = GL_UNSIGNED_BYTE;
			break;
		case texture_format_r16f:
			gl_format = GL_RED;
			gl_type = GL_HALF_FLOAT;
			break;
		case texture_format_r32f:
			gl_format = GL_RED;
			gl_type = GL_FLOAT;
			break;
		case texture_format_rg8i:
			gl_format = GL_RG;
			gl_type = GL_UNSIGNED_BYTE;
			break;
		case texture_format_rg16f:
			gl_format = GL_RG;
			gl_type = GL_HALF_FLOAT;
			break;
		case texture_format_rg32f:
			gl_format = GL_RG;
			gl_type = GL_FLOAT;
			break;
		case texture_format_rgb8i: 
			gl_format = GL_RGB;
			gl_type = GL_UNSIGNED_BYTE;
			break;
		case texture_format_rgb16f:
			gl_format = GL_RGB;
			gl_type = GL_HALF_FLOAT;
			break;
		case texture_format_rgb32f:
			gl_format = GL_RGB;
			gl_type = GL_FLOAT;
			break;
		case texture_format_rgba8i:
			gl_format = GL_RGBA;
			gl_type = GL_UNSIGNED_BYTE;
			break;
		case texture_format_rgba16f:
			gl_format = GL_RGBA;
			gl_type = GL_HALF_FLOAT;
			break;
		case texture_format_rgba32f:
			gl_format = GL_RGBA;
			gl_type = GL_FLOAT;
			break;
	}

	u32 filter = flags & texture_flags_filter_linear ? GL_LINEAR : GL_NEAREST;

	check_gl(glGenTextures(1, &texture->id));
	check_gl(glBindTexture(GL_TEXTURE_2D, texture->id));
	check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
	check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
	check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	check_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	check_gl(glTexImage2D(GL_TEXTURE_2D, 0, gl_format, image->size.x, image->size.y, 0,
		gl_format, gl_type, image->colours));
}

static void deinit_texture(struct video_gl_texture* texture) {
	check_gl(glDeleteTextures(1, &texture->id));
}

struct texture* video_gl_new_texture(const struct image* image, u32 flags, u32 format) {
	struct video_gl_texture* texture = core_calloc(1, sizeof *texture);

	init_texture(texture, image, flags, format);

	return (struct texture*)texture;
}

void video_gl_free_texture(struct texture* texture) {
	deinit_texture((struct video_gl_texture*)texture);
	core_free(texture);
}

v2i video_gl_get_texture_size(const struct texture* texture) {
	return ((struct video_gl_texture*)texture)->size;
}

void video_gl_texture_copy(struct texture* dst_, v2i dst_offset, const struct texture* src_, v2i src_offset, v2i dimensions) {
	struct video_gl_texture* dst = (struct video_gl_texture*)dst_;
	struct video_gl_texture* src = (struct video_gl_texture*)src_;

	//dst_offset.y = dst->size.y - (dst_offset.y + dimensions.y);
	src_offset.y = src->size.y - (src_offset.y + dimensions.y);

	check_gl(glCopyImageSubData(
		src->id, GL_TEXTURE_2D, 0, src_offset.x, src_offset.y, 0,
		dst->id, GL_TEXTURE_2D, 0, dst_offset.x, dst_offset.y, 0,
		dimensions.x, dimensions.y, 1));
}

void video_gl_texture_barrier(struct texture* texture, u32 state) {
	abort_with("Not implemented.");
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

static void init_shader(struct video_gl_shader* shader, const struct shader_header* header, const u8* data) {
	if (header->is_compute) {
		abort_with("Not implemented");
	} else {
		const char* vert_src = (const char*)data + header->raster_header.v_gl_offset;
		const char* frag_src = (const char*)data + header->raster_header.f_gl_offset;

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
}

static void deinit_shader(struct video_gl_shader* shader) {
	check_gl(glDeleteProgram(shader->program));
}

struct shader* video_gl_new_shader(const u8* data, usize data_size) {
	struct video_gl_shader* shader = core_calloc(1, sizeof *shader);

	struct shader_header* header = (struct shader_header*)data;

	if (memcmp("CSH", header->header, 3) != 0) {
		error("Invalid shader data.");
		return null;
	}

	init_shader(shader, header, data);

	return (struct shader*)shader;
}

void video_gl_free_shader(struct shader* shader) {
	deinit_shader((struct video_gl_shader*)shader);
	core_free(shader);
}

static void shader_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct shader_header* header = (struct shader_header*)raw;

	memset(payload, 0, payload_size);

	if (memcmp("CSH", header->header, 2) != 0) {
		error("%s is not a valid shader resource.", filename);
		return;
	}

	init_shader(payload, header, raw);
}

static void shader_on_unload(void* payload, usize payload_size) {
	deinit_shader(payload);
}

static void texture_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct image image;
	init_image_from_raw(&image, raw, raw_size);

	init_texture(payload, &image, *(u32*)udata, texture_format_rgba8i);
}

static void texture_on_unload(void* payload, usize payload_size) {
	deinit_texture(payload);
}

void video_gl_register_resources() {
	reg_res_type("shader", &(struct res_config) {
		.payload_size = sizeof(struct video_gl_shader),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_csh,
		.alt_raw_size = bir_error_csh_size,
		.on_load = shader_on_load,
		.on_unload = shader_on_unload
	});

	reg_res_type("texture", &(struct res_config) {
		.payload_size = sizeof(struct video_gl_texture),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_png,
		.alt_raw_size = bir_error_png_size,
		.on_load = texture_on_load,
		.on_unload = texture_on_unload
	});
}

u32 video_gl_get_draw_call_count() {
	return gctx.draw_call_count;
}
