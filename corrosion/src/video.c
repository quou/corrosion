#include "core.h"
#include "video.h"
#include "video_gl.h"
#include "video_vk.h"

struct video video;

struct framebuffer_val_meta {
	usize attachment_count;
};

struct {
	bool is_init;
	bool is_deinit;
	bool end_called;
	bool has_default_fb;

	struct framebuffer* current_fb;

	table(struct framebuffer*, struct framebuffer_val_meta) fb_meta;
} validation_state;

#define get_api_proc(n_) ( \
	video.api == video_api_vulkan ? cat(video_vk_, n_) : \
	video.api == video_api_opengl ? cat(video_gl_, n_) : null)

#define check_is_init(n_) \
	if (!validation_state.is_init) { \
		error("video." n_ ": Video context not initialised."); \
		ok = false; \
	}

#define check_isnt_null(o_, n_, e_) \
	if ((o_) == null) { \
		error("video." n_ ": " e_); \
		ok = false; \
	}

#define check_is_begin(n_) \
	if (validation_state.end_called) { \
		error("video." n_ ": Mismatched video.begin/video.end. Did you forget to call video.begin?"); \
		ok = false; \
	}

static void validated_init(const struct video_config* config) {
	bool ok = true;

	if (config->clear_colour.r > 1.0f || config->clear_colour.g > 1.0f || config->clear_colour.b > 1.0f || config->clear_colour.a > 1.0f) {
		warning("video.init: clear_color must not be an HDR colour, as the default framebuffer does not support HDR.");
	}

	if (config->clear_colour.r < 0.0f || config->clear_colour.g < 0.0f || config->clear_colour.b < 0.0f || config->clear_colour.a < 0.0f) {
		warning("video.init: Red, green, blue and alpha values of clear_color must not be negative.");
	}

	if (ok) {
		validation_state.is_init = true;
		validation_state.is_deinit = false;
		validation_state.end_called = true;
		get_api_proc(init)(config);
		return;
	}

	abort();
}

static void validated_deinit() {
	bool ok = true;

	check_is_init("deinit");

	if (validation_state.is_deinit) {
		error("video.deinit: Cannot deinit if deinit has already been called.");
		ok = false;
	}
	
	if (ok) {
		validation_state.is_deinit = true;
		get_api_proc(deinit)();

		free_table(validation_state.fb_meta);

		return;
	}

	abort();
}

static void validated_begin() {
	bool ok = true;

	check_is_init("begin");

	if (!validation_state.end_called) {
		error("video.begin: Mismatched video.begin/video.end. Did you forget to call video.end?");
		ok = false;
	}

	if (ok) {	
		validation_state.end_called = false;
		get_api_proc(begin)();
		return;
	}

	abort();
}

static void validated_end() {
	bool ok = true;

	check_is_init("end");
	check_is_begin("end");

	if (ok) {
		validation_state.end_called = true;
		get_api_proc(end)();
		return;
	}

	abort();
}

static struct framebuffer* validated_get_default_fb() {
	bool ok = true;
	bool fatal = false;

	check_is_init("get_default_fb");

	if (ok) {
		return get_api_proc(get_default_fb)();
	}

	abort();
}

static struct framebuffer* validated_new_framebuffer(
	u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {

	bool ok = true;

	check_is_init("new_framebuffer");

	if (validation_state.has_default_fb && flags & framebuffer_flags_default) {
		error("video.new_framebuffer: Cannot have more than one framebuffer created with framebuffer_flags_default.");
		ok = false;
	}

	if (flags & framebuffer_flags_default && attachment_count > 2) {
		error("video.new_framebuffer: Cannot have more than two attachments on the default framebuffer.");
		ok = false;
	}

	if (flags > (framebuffer_flags_fit | framebuffer_flags_default | framebuffer_flags_headless)) {
		error("video.new_framebuffer: Invalid flags.");
		ok = false;
	}

	if (flags & framebuffer_flags_default && flags & framebuffer_flags_headless) {
		error("video.new_framebuffer: Flags cannot be framebuffer_flags_default and framebuffer_flags_headless simultaneously.");
		ok = false;
	}

	if (flags & framebuffer_flags_default && ~flags & framebuffer_flags_fit) {
		warning("video.new_framebuffer: Default framebuffers should be created with framebuffer_flags_fit.");
	}

	v2i window_size = get_window_size();

	if (flags & framebuffer_flags_default && (size.x > window_size.x || size.y > window_size.y)) {
		error("video.new_framebuffer: Default framebuffer mustn't be larger than the window.");
		ok = false;
	}

	if (flags & framebuffer_flags_fit && (size.x > window_size.x || size.y > window_size.y)) {
		warning("video.new_framebuffer: Fitted framebuffers shouldn't be larger than the window.");
	}

	if (size.x < 1 || size.y < 1) {
		error("video.new_framebuffer: `size' mustn't be less than 1x1");
		ok = false;
	}

	for (usize i = 0; i < attachment_count; i++) {
		const struct framebuffer_attachment_desc* desc = attachments + i;

		if (desc->type >= framebuffer_attachment_type_count) {
			error("video.new_framebuffer: Attachment %u (%s): Attachment descriptor type must be"
			" smaller than framebuffer_attachment_type_count.", i, desc->name);
			ok = false;
		}

		if (desc->format >= framebuffer_format_count) {
			error("video.new_framebuffer: Attachment %u (%s): Attachment descriptor format must be"
			" smaller than framebuffer_format_count.", i, desc->name);
			ok = false;
		}

		if (desc->format == framebuffer_format_depth && desc->type != framebuffer_attachment_depth) {
			error("video.new_framebuffer: Attachment %u (%s): If the attachment format is"
			" framebuffer_format_depth, then the type must be framebuffer_attachment_depth.", i, desc->name);
			ok = false;
		}

		if (desc->type == framebuffer_attachment_depth && desc->format != framebuffer_format_depth) {
			error("video.new_framebuffer: Attachment %u (%s): If the attachment type is"
			" framebuffer_attachment_depth, then the format must be framebuffer_format_depth.", i, desc->name);
			ok = false;
		}

		if (desc->type == framebuffer_attachment_colour && (desc->format != framebuffer_format_rgba8i &&
			desc->format != framebuffer_format_rgba16f &&
			desc->format != framebuffer_format_rgba32f)) {

			error("video.new_framebuffer: Attachment %u (%s): If the attachment type is"
			" framebuffer_attachment_colour, then the format must be any one of:"
			" framebuffer_format_rgba8i, framebuffer_format_rgba16f or framebuffer_format_rgba32f.",
			i, desc->name);
			ok = false;
		}

		if ((desc->format == framebuffer_format_rgba8i ||
			desc->format == framebuffer_format_rgba16f ||
			desc->format == framebuffer_format_rgba32f) &&
			desc->type != framebuffer_attachment_colour) {

			error("video.new_framebuffer: Attachment %u (%s): If the attachment format is"
			" any one of: framebuffer_format_rgba8i, framebuffer_format_rgba16f or framebuffer_format_rgba32f,"
			" then the attachment type must framebuffer_attachment_colour.", i, desc->name);
			ok = false;
		}
	}

	if (ok) {
		validation_state.has_default_fb = true;
		struct framebuffer* fb = get_api_proc(new_framebuffer)(flags, size, attachments, attachment_count);

		table_set(validation_state.fb_meta, fb, ((struct framebuffer_val_meta) { attachment_count }));

		return fb;
	}

	abort();
}

static void validated_free_framebuffer(struct framebuffer* fb) {
	bool ok = true;

	check_is_init("free_framebuffer");
	check_isnt_null(fb, "free_framebuffer", "`fb' must be a valid pointer to a framebuffer object.");

	if (ok) {
		table_delete(validation_state.fb_meta, fb);
		get_api_proc(free_framebuffer)(fb);
		return;
	}

	abort();
}

static v2i validated_get_framebuffer_size(const struct framebuffer* fb) {
	bool ok = true;

	check_is_init("get_framebuffer_size");
	check_isnt_null(fb, "get_framebuffer_size", "`fb' must be a valid pointer to a framebuffer object.");

	if (ok) {
		return get_api_proc(get_framebuffer_size)(fb);
	}

	abort();
}

static void validated_resize_framebuffer(struct framebuffer* fb, v2i new_size) {
	bool ok = true;

	check_is_init("resize_framebuffer");
	check_isnt_null(fb, "resize_framebuffer", "`fb' must be a valid pointer to a framebuffer object.");

	if (new_size.x < 1 || new_size.y < 1) {
		error("video.resize_framebuffer: `new_size' mustn't be less than 1x1");
		ok = false;
	}

	if (ok) {
		get_api_proc(resize_framebuffer)(fb, new_size);
		return;
	}

	abort();
}

static void validated_begin_framebuffer(struct framebuffer* fb) {
	bool ok = true;

	check_is_init("begin_framebuffer");
	check_isnt_null(fb, "begin_framebuffer", "`fb' must be a valid pointer to a framebuffer object.");
	check_is_begin("begin_framebuffer");

	if (validation_state.current_fb) {
		error("video.begin_framebuffer: Mismatched video.begin_framebuffer/video.end_framebuffer."
		"Did you forget to call video.end_framebuffer?");
		ok = false;
	}

	if (ok) {
		validation_state.current_fb = fb;
		get_api_proc(begin_framebuffer)(fb);
		return;
	}

	abort();
}

static void validated_end_framebuffer(struct framebuffer* fb) {
	bool ok = true;

	check_is_init("end_framebuffer");
	check_isnt_null(fb, "end_framebuffer", "`fb' must be a valid pointer to a framebuffer object.");
	check_is_begin("begin_framebuffer");

	if (validation_state.current_fb != fb) {
		error("video.end_framebuffer: Mismatched video.begin_framebuffer/video.end_framebuffer");
		ok = false;
	}

	if (ok) {
		validation_state.current_fb = null;
		get_api_proc(end_framebuffer)(fb);
		return;
	}

	abort();
}

static struct texture* validated_get_attachment(struct framebuffer* fb, u32 attachment) {
	bool ok = true;

	check_is_init("get_attachment");
	check_isnt_null(fb, "get_attachment", "`fb' must be a valid pointer to a framebuffer object.");

	struct framebuffer_val_meta* meta = table_get(validation_state.fb_meta, fb);

	if (attachment >= (u32)meta->attachment_count) {
		error("video.get_attachment: `attachment' must be"
			" less than the amount of attachments attached to the given framebuffer.");
		ok = false;
	}

	if (ok) {
		return get_api_proc(get_attachment)(fb, attachment);
	}

	abort();
}

struct pipeline* validated_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config) {

	bool ok = true;

	check_is_init("new_pipeline");
	check_isnt_null(shader, "new_pipeline", "`shader' must be a valid pointer to a shader object.");
	check_isnt_null(framebuffer, "new_pipeline", "`framebuffer' must be a valid pointer to a framebuffer object.");

	if (!attrib_bindings.count || !attrib_bindings.bindings) {
		error("new_pipeline: Graphics pipelines must have more than one attribute binding."
			" If you meant to create a compute pipeline, use `video.new_compute_pipeline' instead.");
		ok = false;
	}

	vector(u32) used_locs = null;
	vector(u32) used_binds = null;

	for (usize i = 0; i < attrib_bindings.count; i++) {
		struct pipeline_attribute_binding* binding = &attrib_bindings.bindings[i];

		if (binding->rate != pipeline_attribute_rate_per_vertex && binding->rate != pipeline_attribute_rate_per_instance) {
			error("video.new_pipeline: Attribute binding %u: Binding rate must be equal to either"
				" pipeline_attribute_rate_per_vertex or pipeline_attribute_rate_per_instance.", binding->binding);
			ok = false;
		}

		for (usize ii = 0; ii < vector_count(used_binds); ii++) {
			if (binding->binding == used_binds[ii]) {
				error("video.new_pipeline: Attribute binding %u cannot be re-used.", binding->binding);
				ok = false;
				break;
			}
		}

		vector_push(used_binds, binding->binding);

		for (usize j = 0; j < binding->attributes.count; j++) {
			struct pipeline_attribute* attrib = &binding->attributes.attributes[j];

			for (usize ii = 0; ii < vector_count(used_locs); ii++) {
				if (attrib->location == used_locs[ii]) {
					error("video.new_pipeline: Attribute binding %u: Location %u cannot be re-used.",
						binding->binding, attrib->location);
					ok = false;
					break;
				}
			}

			if (attrib->type != pipeline_attribute_float && attrib->type != pipeline_attribute_vec2 &&
				attrib->type != pipeline_attribute_vec3  && attrib->type != pipeline_attribute_vec4) {
				error("video.new_pipeline: Attribute binding %u: Location %u: Type must be equal to any one of:"
					" pipeline_attribute_float, pipeline_attribute_vec2, pipeline_attribute_vec3 or pipeline_attribute_vec4",
					binding->binding, attrib->location);
				ok = false;
			}

			vector_push(used_locs, attrib->location);
		}
	}

	free_vector(used_locs);

	vector(u64) used_set_names = null;
	vector(u64) used_desc_names = null;

	for (usize i = 0; i < descriptor_sets.count; i++) {
		struct pipeline_descriptor_set* set = &descriptor_sets.sets[i];

		if (!set->name) {
			error("video.new_pipeline: Descriptor set %u must be named.", i);
			ok = false;
			break;
		}

		u64 set_name_hash = hash_string(set->name);
		for (usize ii = 0; ii < vector_count(used_set_names); ii++) {
			if (used_set_names[ii] == set_name_hash) {
				error("video.new_pipeline: Descriptor set %s: Duplicate set name.", set->name);
				ok = false;
				break;
			}
		}

		vector_clear(used_desc_names);
		vector_clear(used_binds);
		for (usize j = 0; j < set->count; j++) {
			struct pipeline_descriptor* desc = &set->descriptors[j];

			if (!desc->name) {
				error("video.new_pipeline: Descriptor %u on set %s must be named.", j, set->name);
				ok = false;
				break;
			}

			u64 desc_name_hash = hash_string(desc->name);

			for (usize jj = 0; jj < vector_count(used_desc_names); jj++) {
				if (used_desc_names[jj] == desc_name_hash) {
					error("video.new_pipeline: Descriptor %s: Duplicate name.");
					ok = false;
					break;
				}
			}

			for (usize jj = 0; jj < vector_count(used_binds); jj++) {
				if (used_binds[jj] == desc->binding) {
					error("video.new_pipeline: Descriptor %s, on set %s: Duplicate binding %u.", desc->name, set->name, desc->binding);
					ok = false;
					break;
				}
			}

			if (desc->stage != pipeline_stage_vertex && desc->stage != pipeline_stage_fragment) {
				error("video.new_pipeline: Descriptor %s on set %s: Stage must",
					" be equal to either pipeline_stage_vertex or pipeline_stage_fragment", desc->name, set->name);
				ok = false;
			}

			if (desc->resource.type != pipeline_resource_uniform_buffer &&
				desc->resource.type != pipeline_resource_texture &&
				desc->resource.type != pipeline_resource_storage) {
				error("video.new_pipeline: Descriptor %s on set %s: Resource type must"
					" be equal to one of: pipeline_resource_uniform_buffer, pipeline_resource_texture or pipeline_resource_storage",
					desc->name, set->name);
				ok = false;
			}

			if (desc->resource.type == pipeline_resource_texture && desc->resource.texture == null) {
				error("video.new_pipeline: Descriptor %s on set %s: If resource type is pipeline_resource_texture"
					", then resource.texture must be a valid pointer to a texture object.", desc->name, set->name);
				ok = false;
			}

			if (desc->resource.type == pipeline_resource_storage && desc->resource.storage == null) {
				error("video.new_pipeline: Descriptor %s on set %s: If resource type is pipeline_resource_storage"
					", then resource.storage must be a valid pointer to a storage object.", desc->name, set->name);
				ok = false;
			}

			vector_push(used_binds, desc->binding);
			vector_push(used_desc_names, desc_name_hash);
		}
		
		vector_push(used_set_names, set_name_hash);
	}
	
	free_vector(used_binds);
	free_vector(used_set_names);
	free_vector(used_desc_names);

	if (ok) {
		return get_api_proc(new_pipeline)(flags, shader, framebuffer, attrib_bindings, descriptor_sets);
	}

	abort();
}

static struct pipeline* validated_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {

	struct pipeline_config default_config = default_pipeline_config();

	return get_api_proc(new_pipeline_ex)(flags, shader, framebuffer, attrib_bindings, descriptor_sets, &default_config);
}

static void find_procs(u32 api, bool enable_validation) {
#define get_v_proc(n_) \
	enable_validation ? cat(validated_, n_) : get_api_proc(n_)

	video.init   = get_v_proc(init);
	video.deinit = get_v_proc(deinit);

	video.begin = get_v_proc(begin);
	video.end   = get_v_proc(end);

	video.get_default_fb = get_v_proc(get_default_fb);

	video.new_framebuffer      = get_v_proc(new_framebuffer);
	video.free_framebuffer     = get_v_proc(free_framebuffer);
	video.get_framebuffer_size = get_v_proc(get_framebuffer_size);
	video.resize_framebuffer   = get_v_proc(resize_framebuffer);
	video.begin_framebuffer    = get_v_proc(begin_framebuffer);
	video.end_framebuffer      = get_v_proc(end_framebuffer);
	video.get_attachment       = get_v_proc(get_attachment);

	video.new_pipeline                 = get_v_proc(new_pipeline);
	video.new_pipeline_ex              = get_v_proc(new_pipeline_ex);
	video.new_compute_pipeline         = get_api_proc(new_compute_pipeline);
	video.free_pipeline                = get_api_proc(free_pipeline);
	video.begin_pipeline               = get_api_proc(begin_pipeline);
	video.end_pipeline                 = get_api_proc(end_pipeline);
	video.invoke_compute               = get_api_proc(invoke_compute);
	video.recreate_pipeline            = get_api_proc(recreate_pipeline);
	video.bind_pipeline_descriptor_set = get_api_proc(bind_pipeline_descriptor_set);
	video.update_pipeline_uniform      = get_api_proc(update_pipeline_uniform);
	video.pipeline_add_descriptor_set  = get_api_proc(pipeline_add_descriptor_set);
	video.pipeline_change_shader       = get_api_proc(pipeline_change_shader);

	video.new_storage           = get_api_proc(new_storage);
	video.update_storage        = get_api_proc(update_storage);
	video.update_storage_region = get_api_proc(update_storage_region);
	video.copy_storage          = get_api_proc(copy_storage);
	video.storage_barrier       = get_api_proc(storage_barrier);
	video.storage_bind_as       = get_api_proc(storage_bind_as);
	video.free_storage          = get_api_proc(free_storage);
	video.invoke_compute        = get_api_proc(invoke_compute);

	video.new_vertex_buffer    = get_api_proc(new_vertex_buffer);
	video.free_vertex_buffer   = get_api_proc(free_vertex_buffer);
	video.bind_vertex_buffer   = get_api_proc(bind_vertex_buffer);
	video.update_vertex_buffer = get_api_proc(update_vertex_buffer);
	video.copy_vertex_buffer   = get_api_proc(copy_vertex_buffer);

	video.new_index_buffer  = get_api_proc(new_index_buffer);
	video.free_index_buffer = get_api_proc(free_index_buffer);
	video.bind_index_buffer = get_api_proc(bind_index_buffer);

	video.draw         = get_api_proc(draw);
	video.draw_indexed = get_api_proc(draw_indexed);
	video.set_scissor  = get_api_proc(set_scissor);

	video.new_texture      = get_api_proc(new_texture);
	video.free_texture     = get_api_proc(free_texture);
	video.get_texture_size = get_api_proc(get_texture_size);
	video.texture_copy     = get_api_proc(texture_copy);

	video.new_shader  = get_api_proc(new_shader);
	video.free_shader = get_api_proc(free_shader);

	video.get_draw_call_count = get_api_proc(get_draw_call_count);

#undef get_proc
}

void init_video(const struct video_config* config) {
	video.api = config->api;

	if (config->api != video_api_vulkan && config->api != video_api_opengl) {
		abort_with("Invalid video API.");
	}

	if (config->enable_validation) {
		memset(&validation_state, 0, sizeof validation_state);
	}

	switch (config->api) {
	case video_api_vulkan:
		info("Using API: Vulkan.");

		find_procs(config->api, config->enable_validation);

		video.init(config);

		video_vk_register_resources();
		break;
	case video_api_opengl:
		info("Using API: OpenGL ES.");

		find_procs(config->api, config->enable_validation);

		video.init(config);

		video_gl_register_resources();
		break;
	}
}

void deinit_video() {
	video.deinit();
}

m4f get_camera_view(const struct camera* camera) {
	v3f cam_dir = make_v3f(
		cosf(to_rad(camera->rotation.x)) * sinf(to_rad(camera->rotation.y)),
		sinf(to_rad(camera->rotation.x)),
		cosf(to_rad(camera->rotation.x)) * cosf(to_rad(camera->rotation.y))
	);

	return m4f_lookat(camera->position, v3f_add(camera->position, cam_dir), make_v3f(0.0f, 1.0f, 0.0f));
}

m4f get_camera_projection(const struct camera* camera, f32 aspect) {
	return m4f_persp(camera->fov, aspect, camera->near_plane, camera->far_plane);
}

void init_vertex_vector(struct vertex_vector* v, usize element_size, usize initial_capacity) {
	memset(v, 0, sizeof *v);

	v->buffer = video.new_vertex_buffer(null, element_size * initial_capacity, vertex_buffer_flags_dynamic | vertex_buffer_flags_transferable);
	v->capacity = initial_capacity;
	v->element_size = element_size;
}

void deinit_vertex_vector(struct vertex_vector* v) {
	video.free_vertex_buffer(v->buffer);

	memset(v, 0, sizeof *v);
}

void vertex_vector_push(struct vertex_vector* v, void* elements, usize count) {
	usize new_count = v->count + count;
	if (new_count > v->capacity) {
		while (v->capacity < new_count) {
			v->capacity *= 2;
		}

		usize new_size = v->capacity * v->element_size;
		usize old_size = v->count * v->element_size;
		struct vertex_buffer* new_buffer = video.new_vertex_buffer(null, new_size, vertex_buffer_flags_dynamic | vertex_buffer_flags_transferable);
		struct vertex_buffer* old_buffer = v->buffer;
		video.copy_vertex_buffer(new_buffer, 0, old_buffer, 0, old_size);
		video.free_vertex_buffer(old_buffer);

		v->buffer = new_buffer;
	}

	usize size = count * v->element_size;
	usize offset = v->count * v->element_size;

	video.update_vertex_buffer(v->buffer, elements, size, offset);

	v->count += count;
}
