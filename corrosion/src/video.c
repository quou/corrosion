#include "core.h"
#include "video.h"
#include "video_vk.h"

struct video video;

void init_video(u32 api, bool enable_validation) {
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

		video.new_vertex_buffer    = video_vk_new_vertex_buffer;
		video.free_vertex_buffer   = video_vk_free_vertex_buffer;
		video.bind_vertex_buffer   = video_vk_bind_vertex_buffer;
		video.update_vertex_buffer = video_vk_update_vertex_buffer;

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

		video.ortho = video_vk_ortho;

		video_vk_register_resources();
		break;
	default:
		abort_with("Unable to create the video context: Unknown API.");
	}

	video.init(enable_validation);
}

void deinit_video() {
	video.deinit();
}
