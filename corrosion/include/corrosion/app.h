#pragma once

#include <time.h>

#include "common.h"
#include "maths.h"

struct window_config {
	const char* title;
	v2i size;
	bool resizable;
};

struct video_config {
	u32 api;
	bool enable_validation;
	bool enable_vsync;
	v4f clear_colour;
};

struct app_config {
	const char* name;
	struct video_config video_config;
	struct window_config window_config;
};

#ifdef cr_entrypoint

#include "window.h"
#include "res.h"
#include "gizmo.h"
#include "timer.h"

void reconfigure_app(struct app_config config);

bool want_reconfigure;
struct app_config current_config;

extern struct app_config cr_config();
extern void cr_init();
extern void cr_update(f64 ts);
extern void cr_deinit();

static bool run(i32 argc, const char** argv) {
	alloc_init();

	struct app_config* cfg = &current_config;

	init_timer();

	srand((u32)time(0));

	init_window(&cfg->window_config, cfg->video_config.api);

	res_init(argv[0]);

	init_video(&cfg->video_config);

	gizmos_init();

	cr_init();

	u64 now = get_timer(), last = get_timer();
	f64 ts = 0.0;

	bool r = true;

	while (window_open() && r) {
		table_lookup_count = 0;
		heap_allocation_count = 0;

		update_events();

		video.begin(true);
		cr_update(ts);
		video.end(true);

		now = get_timer();
		ts = (f64)(now - last) / (f64)get_timer_frequency();
		last = now;

		r = !want_reconfigure;
	}

	cr_deinit();

	gizmos_deinit();

	res_deinit();

	deinit_video();
	deinit_window();

	leak_check();

	alloc_deinit();

	return r;
}

void reconfigure_app(struct app_config config) {
	current_config = config;
	want_reconfigure = true;
}

i32 main(i32 argc, const char** argv) {
	want_reconfigure = false;

	current_config = cr_config();

	while (!run(argc, argv)) {
		want_reconfigure = false;
	}
}


#endif
