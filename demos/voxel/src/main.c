#include <stdio.h>
#include <time.h>

#include <corrosion/cr.h>

#include "chunk.h"

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct pipeline* draw_pip;
	struct pipeline* blit_pip;

	struct texture* draw_result_colour;
	struct texture* draw_result_depth;

	struct vertex_buffer* vb;

	struct chunk chunk;
	struct storage* voxels;

	struct camera camera;

	v2i old_mouse;
	bool camera_active;
	bool first_move;

	f64 fps_timer;
	char fps_buf[256];

	struct ui* ui;

	struct {
		v2i resolution;
		f32 fov;
		pad(4); v3f camera_position;
		pad(4); m4f view;
		v3f chunk_pos;
		pad(4); v3f chunk_extent;
	} render_data;

	struct {
		v2f image_size;
	} blit_data;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Voxel Engine",
		.video_config = (struct video_config) {
			.api = video_api_vulkan,
#ifdef debug
			.enable_validation = true,
#else
			.enable_validation = false,
#endif
			.clear_colour = make_rgba(0x000000, 255)
		},
		.window_config = (struct window_config) {
			.title = "Voxel Engine",
			.size = make_v2i(1920, 1080),
			.resizable = true
		}
	};
}

static void create_resources() {
	const struct shader* blit_shader    = load_shader("shaders/blit.csh");
	const struct shader* draw_shader    = load_shader("shaders/raytrace.csh");

	struct image i = {
		.size = get_window_size()
	};
	app.draw_result_colour   = video.new_texture(&i,
		texture_flags_filter_none | texture_flags_clamp | texture_flags_storage, texture_format_rgba16f);
	app.draw_result_depth = video.new_texture(&i,
		texture_flags_filter_none | texture_flags_clamp | texture_flags_storage, texture_format_r32f);

	app.draw_pip = video.new_compute_pipeline(
		pipeline_flags_none, draw_shader,
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "output_colour",
							.binding = 0,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_texture_storage,
								.texture = app.draw_result_colour
							}
						},
						{
							.name = "output_depth",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_texture_storage,
								.texture = app.draw_result_depth
							}
						},
						{
							.name = "voxels",
							.binding = 2,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.voxels
							}
						},
						{
							.name = "RenderData",
							.binding = 3,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.render_data
							}
						}
					},
					.count = 4
				}
			},
			.count = 1,
		});

	app.blit_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		blit_shader,
		video.get_default_fb(),
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name = "position",
								.location = 0,
								.offset = offsetof(struct vertex, position),
								.type = pipeline_attribute_vec2
							},
							{
								.name = "uv",
								.location = 1,
								.offset = offsetof(struct vertex, uv),
								.type = pipeline_attribute_vec2
							}
						},
						.count = 2
					},
					.stride = sizeof(struct vertex),
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = 0
				}
			},
			.count = 1
		},
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "image",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = app.draw_result_colour
							}
						},
						{
							.name = "Config",
							.binding = 1,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.blit_data
							}
						}
					},
					.count = 2
				}
			},
			.count = 1
		}
	);
}

static void destroy_resources() {
	video.free_pipeline(app.blit_pip);
	video.free_pipeline(app.draw_pip);
	video.free_texture(app.draw_result_colour);
	video.free_texture(app.draw_result_depth);
}

static void on_resize(const struct window_event* event) {
	destroy_resources();
	create_resources();
}

static f32 rand_flt() {
	return (f32)rand() / (f32)RAND_MAX;
}

void cr_init() {
	srand(time(0));

	app.fps_timer = 0.0;

	ui_init();

	app.ui = new_ui(video.get_default_fb());

	app.camera = (struct camera) {
		.fov = 70.0f,
		.position = make_v3f(-5.0f, 7.0f, -5.0f),
		.rotation = make_v3f(-40.0f, -140.0f, 0.0f),
		.near_plane = 0.0f,
		.far_plane = 1000.0f
	};

	window_event_subscribe(window_event_size_changed, on_resize);

	struct image slices = { 0 };
	u8* raw_image;
	usize raw_image_size;
	if (!read_raw("res/world.png", &raw_image, &raw_image_size)) {
		abort();
	}

	init_image_from_raw(&slices, raw_image, raw_image_size);

	core_free(raw_image);

	init_chunk_from_slices(&app.chunk, make_v3i(0, 0, 0), &slices);

	deinit_image(&slices);

	app.voxels = video.new_storage(storage_flags_cpu_writable, chunk_size(&app.chunk), null);
	copy_chunk_to_storage(app.voxels, &app.chunk, 0);

	create_resources();

	struct vertex verts[] = {
		{ { -1.0,   1.0 }, { 0.0f, 0.0f } },
		{ { -1.0,  -3.0 }, { 0.0f, 2.0f } },
		{ {  3.0,   1.0 }, { 2.0f, 0.0f } }
	};

	app.vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);

	app.camera_active = false;
	app.first_move = true;
}

void cr_update(f64 ts) {
	app.fps_timer += ts;

	ui_begin(app.ui);

	if (mouse_btn_just_pressed(mouse_btn_right)) {
		lock_mouse(true);
		app.camera_active = true;
		app.first_move = true;
	}

	if (mouse_btn_just_released(mouse_btn_right)) {
		lock_mouse(false);
		app.camera_active = false;
	}

	if (app.camera_active) {
		struct camera* cam = &app.camera;

		f32 camera_speed = 3.0f;
		if (key_pressed(key_shift)) {
			camera_speed = 20.0f;
		}

		v2i mouse_pos = get_mouse_pos();

		i32 change_x = app.old_mouse.x - mouse_pos.x;
		i32 change_y = app.old_mouse.y - mouse_pos.y;

		if (app.first_move) {
			app.old_mouse = mouse_pos;
			change_x = change_y = 0;
			app.first_move = false;
		}

		cam->rotation.y -= (f32)change_x * 0.01f;
		cam->rotation.x += (f32)change_y * 0.01f;

		if (cam->rotation.x >= 89.0f) {
			cam->rotation.x = 89.0f;
		}

		if (cam->rotation.x <= -89.0f) {
			cam->rotation.x = -89.0f;
		}

		app.old_mouse = mouse_pos;

		v3f cam_dir = make_v3f(
			cosf(to_rad(cam->rotation.x)) * sinf(to_rad(cam->rotation.y)),
			sinf(to_rad(cam->rotation.x)),
			cosf(to_rad(cam->rotation.x)) * cosf(to_rad(cam->rotation.y))
		);

		if (key_pressed(key_S)) {
			cam->position = v3f_add(cam->position, v3f_scale(v3f_scale(cam_dir, camera_speed), (f32)ts));
		}

		if (key_pressed(key_W)) {
			cam->position = v3f_sub(cam->position, v3f_scale(v3f_scale(cam_dir, camera_speed), (f32)ts));
		}

		if (key_pressed(key_D)) {
			cam->position = v3f_add(cam->position, v3f_scale(v3f_scale(v3f_cross(cam_dir, make_v3f(0.0f, 1.0f, 0.0f)), camera_speed), (f32)ts));
		}

		if (key_pressed(key_A)) {
			cam->position = v3f_sub(cam->position, v3f_scale(v3f_scale(v3f_cross(cam_dir, make_v3f(0.0f, 1.0f, 0.0f)), camera_speed), (f32)ts));
		}
	}

	app.fps_timer += ts;
	if (app.fps_timer >= 1.0) {
		app.fps_timer = 0.0;
		snprintf(app.fps_buf, sizeof app.fps_buf, "FPS: %g", 1.0 / ts);
	}

	ui_label(app.ui, app.fps_buf);

	ui_end(app.ui);

	app.render_data.fov = to_rad(app.camera.fov);
	app.render_data.resolution = get_window_size();
	app.render_data.camera_position = app.camera.position;
	app.render_data.view = get_camera_view(&app.camera);
	app.render_data.chunk_pos = make_v3f(app.chunk.position.x, app.chunk.position.y, app.chunk.position.z);
	app.render_data.chunk_extent = make_v3f(app.chunk.extent.x, app.chunk.extent.y, app.chunk.extent.z);

	app.blit_data.image_size = make_v2f((f32)get_window_size().x, (f32)get_window_size().y);

	video.texture_barrier(app.draw_result_colour,   texture_state_shader_write);
	video.texture_barrier(app.draw_result_depth, texture_state_shader_write);

	video.update_pipeline_uniform(app.draw_pip, "primary", "RenderData", &app.render_data);
	video.update_pipeline_uniform(app.blit_pip, "primary", "Config", &app.blit_data);

	v2i texture_size = video.get_texture_size(app.draw_result_colour);

	video.begin_pipeline(app.draw_pip);
		video.bind_pipeline_descriptor_set(app.draw_pip, "primary", 0);
		video.invoke_compute(make_v3u(
			(u32)((f32)texture_size.x / 16.0f + 1.0f),
			(u32)((f32)texture_size.y / 16.0f + 1.0f),
			1));
	video.end_pipeline(app.draw_pip);

	video.texture_barrier(app.draw_result_colour,   texture_state_shader_graphics_read);
	video.texture_barrier(app.draw_result_depth, texture_state_shader_graphics_read);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.blit_pip);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_pipeline_descriptor_set(app.blit_pip, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.blit_pip);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	deinit_chunk(&app.chunk);

	destroy_resources();

	video.free_storage(app.voxels);
	
	video.free_vertex_buffer(app.vb);

	free_ui(app.ui);
	ui_deinit();
}
