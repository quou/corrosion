#include "core.h"
#include "video.h"
#include "video_vk.h"

struct video video;

struct {
	bool is_init;
	bool is_deinit;
	bool end_called;
	bool has_default_fb;

	struct framebuffer* current_fb;
} validation_state;

#define get_api_proc(n_) \
	(video.api == video_api_vulkan ? cat(video_vk_, n_) : null)

#define check_is_init(n_) \
	if (!validation_state.is_init) { \
		error("video." n_ ": Video context not initialised."); \
		ok = false; \
	}

#define check_is_begin(n_) \
	if (validation_state.end_called) { \
		error("video." n_ ": Mismatched video.begin/video.end. Did you forget to call video.begin?"); \
		ok = false; \
	}

static void validated_init(bool enable_validation, v4f clear_colour) {
	bool ok = true;

	if (clear_colour.r > 1.0f || clear_colour.g > 1.0f || clear_colour.b > 1.0f || clear_colour.a > 1.0f) {
		warning("video.init: clear_color must not be an HDR colour, as the default framebuffer does not support HDR.");
	}

	if (clear_colour.r < 0.0f || clear_colour.g < 0.0f || clear_colour.b < 0.0f || clear_colour.a < 0.0f) {
		warning("video.init: Red, green, blue and alpha values of clear_color must not be negative.");
	}

	if (ok) {
		validation_state.is_init = true;
		validation_state.is_deinit = false;
		validation_state.end_called = true;
		get_api_proc(init)(enable_validation, clear_colour);
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
		error("video.new_framebuffer: Flags cannot be framebuffer+flags_default and framebuffer_flags_headless simultaneously.");
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
		return get_api_proc(new_framebuffer)(flags, size, attachments, attachment_count);
	}

	abort();
}

static void validated_free_framebuffer(struct framebuffer* fb) {
	bool ok = true;

	check_is_init("free_framebuffer");

	if (!fb) {
		error("video.free_framebuffer: `fb' must be a valid pointer to a framebuffer.");
		ok = false;
	}

	if (ok) {
		get_api_proc(free_framebuffer)(fb);
		return;
	}

	abort();
}

static v2i validated_get_framebuffer_size(const struct framebuffer* fb) {
	bool ok = true;

	check_is_init("get_framebuffer_size");

	if (!fb) {
		error("video.get_framebuffer_size: `fb' must be a valid pointer to a framebuffer.");
		ok = false;
	}

	if (ok) {
		return get_api_proc(get_framebuffer_size)(fb);
	}

	abort();
}

static void validated_resize_framebuffer(struct framebuffer* fb, v2i new_size) {
	bool ok = true;

	check_is_init("resize_framebuffer");

	if (!fb) {
		error("video.resize_framebuffer: `fb' must be a valid pointer to a framebuffer.");
		ok = false;
	}

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

	video.new_pipeline                 = video_vk_new_pipeline;
	video.free_pipeline                = video_vk_free_pipeline;
	video.begin_pipeline               = video_vk_begin_pipeline;
	video.end_pipeline                 = video_vk_end_pipeline;
	video.recreate_pipeline            = video_vk_recreate_pipeline;
	video.bind_pipeline_descriptor_set = video_vk_bind_pipeline_descriptor_set;
	video.update_pipeline_uniform      = video_vk_update_pipeline_uniform;
	video.pipeline_add_descriptor_set  = video_vk_pipeline_add_descriptor_set;
	video.pipeline_change_shader       = video_vk_pipeline_change_shader;

	video.new_vertex_buffer    = video_vk_new_vertex_buffer;
	video.free_vertex_buffer   = video_vk_free_vertex_buffer;
	video.bind_vertex_buffer   = video_vk_bind_vertex_buffer;
	video.update_vertex_buffer = video_vk_update_vertex_buffer;
	video.copy_vertex_buffer   = video_vk_copy_vertex_buffer;

	video.new_index_buffer  = video_vk_new_index_buffer;
	video.free_index_buffer = video_vk_free_index_buffer;
	video.bind_index_buffer = video_vk_bind_index_buffer;

	video.draw         = video_vk_draw;
	video.draw_indexed = video_vk_draw_indexed;
	video.set_scissor  = video_vk_set_scissor;

	video.new_texture      = video_vk_new_texture;
	video.free_texture     = video_vk_free_texture;
	video.get_texture_size = video_vk_get_texture_size;
	video.texture_copy     = video_vk_texture_copy;

	video.new_shader  = video_vk_new_shader;
	video.free_shader = video_vk_free_shader;

	video.ortho = video_vk_ortho;
	video.persp = video_vk_persp;

	video.get_draw_call_count = video_vk_get_draw_call_count;

#undef get_proc
}

void init_video(u32 api, bool enable_validation, v4f clear_colour) {
	video.api = api;

	if (api != video_api_vulkan) {
		abort_with("Invalid video API.");
	}

	if (enable_validation) {
		memset(&validation_state, 0, sizeof validation_state);
	}

	switch (api) {
	case video_api_vulkan:
		info("Using API: Vulkan.");

		find_procs(api, enable_validation);

		video_vk_register_resources();
		break;
	}

	video.init(enable_validation, clear_colour);

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
	return video.persp(camera->fov, aspect, camera->near_plane, camera->far_plane);
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
