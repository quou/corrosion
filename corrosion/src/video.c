#include "core.h"
#include "video.h"
#include "video_vk.h"

struct video video;

void init_video(u32 api, bool enable_validation, v4f clear_colour) {
	video.api = api;

	switch (api) {
	case video_api_vulkan:
		info("Using API: Vulkan.");

		video.init   = video_vk_init;
		video.deinit = video_vk_deinit;

		video.begin = video_vk_begin;
		video.end   = video_vk_end;

		video.get_default_fb = video_vk_get_default_fb;

		video.new_framebuffer      = video_vk_new_framebuffer;
		video.free_framebuffer     = video_vk_free_framebuffer;
		video.get_framebuffer_size = video_vk_get_framebuffer_size;
		video.resize_framebuffer   = video_vk_resize_framebuffer;
		video.begin_framebuffer    = video_vk_begin_framebuffer;
		video.end_framebuffer      = video_vk_end_framebuffer;

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

		video_vk_register_resources();
		break;
	default:
		abort_with("Unable to create the video context: Unknown API.");
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
