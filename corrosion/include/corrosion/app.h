#pragma once

#include <time.h>

#include "common.h"
#include "maths.h"

struct window_config {
	const char* title;
	v2i size;
	bool resizable;
	bool fullscreen;
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

#ifdef __EMSCRIPTEN__
extern void window_emscripten_run();
#endif

struct {
	f64 ts;
	u64 now, last;
	bool r;
} _impl__cr__app;

void __cr_app_update() {
	table_lookup_count = 0;
	heap_allocation_count = 0;

	update_events();

	video.begin(true);
	cr_update(_impl__cr__app.ts);
	video.end(true);

	_impl__cr__app.now = get_timer();
	_impl__cr__app.ts = (f64)(_impl__cr__app.now - _impl__cr__app.last) / (f64)get_timer_frequency();
	_impl__cr__app.last = _impl__cr__app.now;

	_impl__cr__app.r = !want_reconfigure;
}

static bool run(i32 argc, const char** argv) {
	struct app_config* cfg = &current_config;

	init_timer();

	srand((u32)time(0));

	init_window(&cfg->window_config, cfg->video_config.api);

	res_init(argv[0]);

	init_video(&cfg->video_config);

	gizmos_init();

	cr_init();

	_impl__cr__app.now = get_timer();
	_impl__cr__app.last = get_timer();
	_impl__cr__app.ts = 0.0;

	_impl__cr__app.r = true;

#ifdef __EMSCRIPTEN__
	window_emscripten_run();
#else
	while (window_open() && _impl__cr__app.r) {
		__cr_app_update();
	}
#endif

	cr_deinit();

	gizmos_deinit();

	res_deinit();

	deinit_video();
	deinit_window();

	return _impl__cr__app.r;
}

void reconfigure_app(struct app_config config) {
	current_config = config;
	want_reconfigure = true;
}

i32 main(i32 argc, const char** argv) {
	alloc_init();

#ifndef __EMSCRIPTEN__
	want_reconfigure = false;

	current_config = cr_config();

	while (!run(argc, argv)) {
		want_reconfigure = false;
	}
#else
	current_config = cr_config();

	run(argc, argv);
#endif

	leak_check();

	alloc_deinit();
}


#endif
