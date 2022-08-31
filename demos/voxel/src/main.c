#include <stdio.h>

#include <corrosion/cr.h>

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct pipeline* process_pip;
	struct pipeline* draw_pip;

	struct vertex_buffer* vb;

	struct camera camera;

	v2i old_mouse;
	bool camera_active;
	bool first_move;

	f64 fps_timer;
	char fps_buf[256];

	struct ui* ui;

	struct {
		v2i size;
	} config;

	struct {
		v2f resolution;
		f32 fov;
		pad(4); v3f camera_position;
		pad(4); m4f camera;
	} render_data;
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

void cr_init() {
	app.fps_timer = 0.0;

	ui_init();

	app.ui = new_ui(video.get_default_fb());

	app.camera = (struct camera) {
		.fov = 70.0f,
		.position = make_v3f(3.0f, 0.0f, 3.0f),
		.rotation = make_v3f(0.0f, 45.0f, 0.0f)
	};

	const struct shader* raymarch_shader = load_shader("shaders/raymarch.csh");

	app.draw_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		raymarch_shader,
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
							.name = "RenderData",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.render_data
							}
						}
					},
					.count = 1
				}
			},
			.count = 1
		}
	);


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

	gizmo_line2d(make_v2f(0.0f, 0.0f), make_v2f(100.0f, 100.0f));

	app.config.size = get_window_size();

	app.render_data.fov = to_rad(app.camera.fov);
	app.render_data.resolution = make_v2f(app.config.size.x, app.config.size.y);
	app.render_data.camera_position = app.camera.position;
	app.render_data.camera = get_camera_view(&app.camera);

	video.update_pipeline_uniform(app.draw_pip, "primary", "RenderData", &app.render_data);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.draw_pip);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_pipeline_descriptor_set(app.draw_pip, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.draw_pip);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.draw_pip);
	
	video.free_vertex_buffer(app.vb);

	free_ui(app.ui);
	ui_deinit();
}
