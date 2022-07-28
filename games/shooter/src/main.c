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

void cr_init() {
	app.renderer = new_renderer();

	app.world = new_world(app.renderer);

	app.monkey = new_entity(app.world, eb_mesh);
}

void cr_update(f64 ts) {
	update_world(app.world, ts);

	video.begin_framebuffer(video.get_default_fb());
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_world(app.world);
	free_renderer(app.renderer);
}
