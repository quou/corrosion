#include <time.h>

#include "app.h"
#include "gizmo.h"
#include "res.h"
#include "timer.h"
#include "video.h"
#include "window.h"

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

	srand(time(0));

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
		
		video.begin();
		cr_update(ts);
		video.end();

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
