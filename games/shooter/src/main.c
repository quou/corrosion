#include <corrosion/cr.h>

#include "entity.h"
#include "renderer.h"

struct {
	struct world* world;

	struct entity* monkey;

	struct renderer* renderer;
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
	register_renderer_resources();

	app.renderer = new_renderer(video.get_default_fb());

	app.world = new_world(app.renderer);

	for (usize i = 0; i < 500; i++) {
		app.monkey = new_entity(app.world, eb_mesh | eb_spin);
		app.monkey->transform = m4f_mul(m4f_translation(make_v3f(rand_flt() * 500.0f, rand_flt() * 500.0f, rand_flt() * 500.0f)), m4f_rotation(euler(make_v3f(0.0f, rand_flt() * 360.0f, 0.0f))));

		app.monkey->model = load_model("meshes/monkey.fbx");
	}
}

void cr_update(f64 ts) {
	renderer_begin(app.renderer);

	update_world(app.world, ts);

	renderer_end(app.renderer);

	video.begin_framebuffer(video.get_default_fb());
		renderer_finalise(app.renderer, &app.world->camera);
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_world(app.world);
	free_renderer(app.renderer);
}
