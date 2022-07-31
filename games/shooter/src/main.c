#include <stdio.h>
#include <time.h>

#include <corrosion/cr.h>

#include "entity.h"
#include "renderer.h"

struct {
	struct world* world;

	struct renderer* renderer;

	struct ui* ui;

	v2i old_mouse;
	bool camera_active;
	bool first_move;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Shooter",
		.video_config = (struct video_config) {
			.api = video_api_vulkan,
			.enable_validation = true,
			.clear_colour = make_rgba(0x000000, 255)
		},
		.window_config = (struct window_config) {
			.title = "Shooter",
			.size = make_v2i(800, 600),
			.resizable = true
		}
	};
}

static f32 rand_flt() {
	return (f32)rand() / (f32)RAND_MAX;
}

void cr_init() {
	ui_init();

	app.ui = new_ui(video.get_default_fb());

	register_renderer_resources();

	app.renderer = new_renderer(video.get_default_fb());

	app.world = new_world(app.renderer);

	srand(time(null));

	for (usize i = 0; i < 30000; i++) {
		struct entity* monkey = new_entity(app.world, eb_mesh | eb_spin);
		monkey->transform = m4f_translation(make_v3f(rand_flt() * 10000.0f, rand_flt() * 10000.0f, rand_flt() * 10000.0f));
		monkey->transform = m4f_mul(monkey->transform, m4f_rotation(euler(make_v3f(0.0f, rand_flt() * 360.0f, 0.0f))));

		f32 v = rand_flt();

		if (v > 0.6666f) {
			monkey->model = load_model("meshes/monkey.fbx");
		} else if (v > 0.3333f) {
			monkey->model = load_model("meshes/sphere.fbx");
		} else {
			monkey->model = load_model("meshes/torus.fbx");
		}

		v = rand_flt();
		if (v > 0.6666f) {
			monkey->material.diffuse_map = load_texture("textures/bricks_diffuse.png", texture_flags_filter_linear);
		} else if (v > 0.3333f) {
			monkey->material.diffuse_map = load_texture("textures/cobble_diffuse.png", texture_flags_filter_linear);
		} else {
			monkey->material.diffuse_map = load_texture("textures/wood_diffuse.png", texture_flags_filter_linear);
		}

		monkey->spin_speed = rand_flt() * 100.0f;
		monkey->material.diffuse = make_v3f(rand_flt() * 2.0f, rand_flt() * 2.0f, rand_flt() * 2.0f);
	}

	app.camera_active = false;
	app.first_move = true;
}

void cr_update(f64 ts) {
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
		struct camera* cam = &app.world->camera;

		f32 camera_speed = 300.0f;
		if (key_pressed(key_shift)) {
			camera_speed = 2000.0f;
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
		cam->rotation.x -= (f32)change_y * 0.01f;

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
			cam->position = v3f_sub(cam->position, v3f_scale(v3f_scale(cam_dir, camera_speed), (f32)ts));
		}

		if (key_pressed(key_W)) {
			cam->position = v3f_add(cam->position, v3f_scale(v3f_scale(cam_dir, camera_speed), (f32)ts));
		}

		if (key_pressed(key_D)) {
			cam->position = v3f_add(cam->position, v3f_scale(v3f_scale(v3f_cross(cam_dir, make_v3f(0.0f, 1.0f, 0.0f)), camera_speed), (f32)ts));
		}

		if (key_pressed(key_A)) {
			cam->position = v3f_sub(cam->position, v3f_scale(v3f_scale(v3f_cross(cam_dir, make_v3f(0.0f, 1.0f, 0.0f)), camera_speed), (f32)ts));
		}
	}

	renderer_begin(app.renderer);

	update_world(app.world, ts);

	renderer_end(app.renderer);

	ui_begin(app.ui);

	static char fps_buf[256] = "";
	static char ts_buf[256] = "";
	static char mem_buf[128];
	sprintf(mem_buf, "Memory Usage (MIB): %g", (f64)core_get_memory_usage() / 1024.0 / 1024.0);
	static f64 fps_timer = 0.9;

	fps_timer += ts;
	if (fps_timer >= 1.0) {
		fps_timer = 0;
		sprintf(fps_buf, "Framerate: %g", 1.0 / ts);
		sprintf(ts_buf, "Timestep: %g", ts);
	}

	ui_label(app.ui, fps_buf);
	ui_label(app.ui, ts_buf);
	ui_label(app.ui, mem_buf);

	ui_end(app.ui);

	video.begin_framebuffer(video.get_default_fb());
		renderer_finalise(app.renderer, &app.world->camera);
		ui_draw(app.ui);
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_world(app.world);
	free_renderer(app.renderer);
	free_ui(app.ui);

	ui_deinit();
}
