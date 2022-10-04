#include <stdio.h>
#include <time.h>

#define cr_entrypoint

#include <corrosion/cr.h>

#define volume_texture_size make_v3i(128, 128, 128)

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct pipeline* pipeline;

	struct texture* v_tex;

	struct vertex_buffer* vb;

	f64 fps_timer;
	char fps_buf[256];

	struct ui* ui;

	bool first_frame;

	v2i old_mouse;
	bool camera_active;
	bool first_move;

	struct camera camera;

	struct {
		f32 time;
		f32 threshold;
		v2f resolution;
		m4f view;
		v3f camera_position;
	} config;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Volumetric Rendering",
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
			.title = "Volumetric Rendering",
			.size = make_v2i(1920, 1080),
			.resizable = true
		}
	};
}

static void shift_and_gen(v3f* points, u32 count, u32 offset, v3f dir) {
	for (u32 i = 0; i < count; i++) {
		points[offset + i] = v3f_add(points[i], dir);
	}
}

/* Generate a volumetric texture using worley noise and a compute shader. */
static void generate_volume_data() {
	u32 point_count = 256;
	u32 tile_count = 23;
	u32 tiled_pc = point_count * tile_count;

	vector(v3f) gen_points = null;
	vector_allocate(gen_points, tiled_pc);

	for (u32 i = 0; i < point_count; i++) {
		vector_push(gen_points,
			make_v3f(
				random_float(0.0f, 1.0f),
				random_float(0.0f, 1.0f),
				random_float(0.0f, 1.0f)
			));
	}

	shift_and_gen(gen_points, point_count, point_count * 1,  make_v3f(-1.0f,  0.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 2,  make_v3f( 1.0f,  0.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 3,  make_v3f( 0.0f, -1.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 4,  make_v3f( 0.0f,  1.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 5,  make_v3f( 0.0f,  0.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 6,  make_v3f( 0.0f,  0.0f,  1.0f));

	shift_and_gen(gen_points, point_count, point_count * 7,  make_v3f( 0.0f, -1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 8,  make_v3f( 0.0f, -1.0f,  1.0f));
	shift_and_gen(gen_points, point_count, point_count * 9,  make_v3f( 1.0f, -1.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 10, make_v3f(-1.0f, -1.0f,  0.0f));

	shift_and_gen(gen_points, point_count, point_count * 11, make_v3f( 0.0f,  1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 12, make_v3f( 0.0f,  1.0f,  1.0f));
	shift_and_gen(gen_points, point_count, point_count * 13, make_v3f( 1.0f,  1.0f,  0.0f));
	shift_and_gen(gen_points, point_count, point_count * 14, make_v3f(-1.0f,  1.0f,  0.0f));

	shift_and_gen(gen_points, point_count, point_count * 15, make_v3f( 1.0f, -1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 16, make_v3f( 1.0f, -1.0f,  1.0f));
	shift_and_gen(gen_points, point_count, point_count * 17, make_v3f(-1.0f, -1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 18, make_v3f(-1.0f, -1.0f,  1.0f));

	shift_and_gen(gen_points, point_count, point_count * 19, make_v3f( 1.0f,  1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 20, make_v3f( 1.0f,  1.0f,  1.0f));
	shift_and_gen(gen_points, point_count, point_count * 21, make_v3f(-1.0f,  1.0f, -1.0f));
	shift_and_gen(gen_points, point_count, point_count * 22, make_v3f(-1.0f,  1.0f,  1.0f));

	struct storage* points = video.new_storage(storage_flags_none, tiled_pc * sizeof(v3f), gen_points);

	free_vector(gen_points);

	const struct shader* shader = load_shader("shaders/volume.csh");

	struct {
		v3f image_size;
		u32 point_count;
	} config;

	config.point_count = tiled_pc;
	config.image_size = make_v3f(volume_texture_size.x, volume_texture_size.y, volume_texture_size.z);

	struct pipeline* pipeline = video.new_compute_pipeline(pipeline_flags_none, shader,
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "config",
							.binding = 0,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof config
							}
						},
						{
							.name = "input_points",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = points
							}
						},
						{
							.name = "output_texture",
							.binding = 2,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_texture_storage,
								.texture = app.v_tex
							}
						}
					},
					.count = 3
				}
			},
			.count = 1
		});

	video.update_pipeline_uniform(pipeline, "primary", "config", &config);

	video.texture_barrier(app.v_tex, texture_state_shader_write);

	video.begin_pipeline(pipeline);
		video.bind_pipeline_descriptor_set(pipeline, "primary", 0);
		video.invoke_compute(make_v3u(volume_texture_size.x / 8, volume_texture_size.y / 8, volume_texture_size.z / 8));
	video.end_pipeline(pipeline);

	video.texture_barrier(app.v_tex, texture_state_shader_graphics_read);

	video.free_storage(points);
	video.free_pipeline(pipeline);
}

void cr_init() {
	app.fps_timer = 0.0;

	ui_init();

	app.ui = new_ui(video.get_default_fb());

	struct vertex verts[] = {
		{ { -1.0,   1.0 }, { 0.0f, 0.0f } },
		{ { -1.0,  -3.0 }, { 0.0f, 2.0f } },
		{ {  3.0,   1.0 }, { 2.0f, 0.0f } }
	};

	app.vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);
	app.v_tex = video.new_texture_3d(volume_texture_size,
		texture_flags_repeat | texture_flags_filter_linear | texture_flags_storage,
		texture_format_r8i);

	struct shader* shader = load_shader("shaders/draw.csh");

	app.pipeline = video.new_pipeline(
		pipeline_flags_draw_tris,
		shader,
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
							.name = "volume",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = app.v_tex
							}
						},
						{
							.name = "config",
							.binding = 1,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof(app.config)
							}
						}
					},
					.count = 2
				}
			},
			.count = 1
		}
	);

	app.first_frame = true;

	app.config.threshold = 0.9f;
	app.camera.position = make_v3f(0.0f, 0.0f, 0.0f);
	app.camera.fov = 70.0f;
	app.camera.near_plane = 0.1f;
	app.camera.far_plane = 100.0f;
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

	if (app.fps_timer >= 1.0) {
		app.fps_timer = 0.0;
		snprintf(app.fps_buf, sizeof app.fps_buf, "FPS: %g", 1.0 / ts);
	}

	ui_label(app.ui, app.fps_buf);

	ui_knob(app.ui, &app.config.threshold, 0.0f, 1.0f);

	ui_end(app.ui);

	if (app.first_frame) {
		generate_volume_data();
		app.first_frame = false;
	}

	v2i win_size = get_window_size();

	app.config.resolution = make_v2f(win_size.x, win_size.y);
	app.config.time += (f32)ts;
	app.config.camera_position = app.camera.position;
	app.config.view = get_camera_view(&app.camera);

	video.update_pipeline_uniform(app.pipeline, "primary", "config", &app.config);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.pipeline);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_pipeline_descriptor_set(app.pipeline, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.pipeline);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.pipeline);
	video.free_texture(app.v_tex);
	
	video.free_vertex_buffer(app.vb);

	free_ui(app.ui);
	ui_deinit();
}
