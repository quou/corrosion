#include "app.h"

#include "gizmo.h"
#include "res.h"
#include "timer.h"
#include "video.h"
#include "window.h"

extern struct app_config cr_config();
extern void cr_init();
extern void cr_update(f64 ts);
extern void cr_deinit();

i32 main(i32 argc, const char** argv) {
	alloc_init();

	struct app_config cfg = cr_config();

	init_timer();

	init_window(&cfg.window_config, cfg.video_config.api);

	res_init(argv[0]);

	init_video(cfg.video_config.api, cfg.video_config.enable_validation, cfg.video_config.clear_colour);

	gizmos_init();

	cr_init();

	u64 now = get_timer(), last = get_timer();
	f64 ts = 0.0;

	while (window_open()) {
		update_events();
		
		video.begin();
		cr_update(ts);
		video.end();

		now = get_timer();
		ts = (f64)(now - last) / (f64)get_timer_frequency();
		last = now;
	}

	cr_deinit();

	gizmos_deinit();

	res_deinit();

	deinit_video();
	deinit_window();

	leak_check();

	alloc_deinit();
}
