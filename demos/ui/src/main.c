#include <stdio.h>

#define cr_entrypoint

#include <corrosion/cr.h>

struct {
	struct ui* ui;
	struct ui_stylesheet* stylesheet;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "UI Demo",
		.video_config = (struct video_config) {
			.api = video_best_api(video_feature_base),
#ifdef debug
			.enable_validation = true,
#else
			.enable_validation = false,
#endif
			.clear_colour = make_rgba(0xff0000, 255)
		},
		.window_config = (struct window_config) {
			.title = "UI Demo",
			.size = make_v2i(800, 600),
			.resizable = true
		}
	};
}

void cr_init() {
	ui_init();

	app.ui = new_ui(video.get_default_fb());

	app.stylesheet = load_stylesheet("stylesheet.dt", null);
}

void cr_update(f64 ts) {
	static char fps_buf[32];
	static f64 fps_timer = 0.0;
	
	fps_timer += ts;
	if (fps_timer > 1.0) {
		fps_timer = 0;
		sprintf(fps_buf, "%g", 1.0 / ts);
	}

	ui_begin(app.ui);

	ui_stylesheet(app.ui, app.stylesheet);

	ui_begin_container(app.ui, make_v4f(0.0f, 0.0f, 1.0f, 1.0f), true);

	ui_columns(app.ui, 2, (f32[]) { 0.2f, 0.7f });
	ui_label(app.ui, "API:");
	ui_label(app.ui, video.get_api_name());

	ui_label(app.ui, "FPS:");
	ui_label(app.ui, fps_buf);

	ui_label(app.ui, "Emscripten?");
#ifdef __EMSCRIPTEN__
	ui_label(app.ui, "Yes.");
#else
	ui_label(app.ui, "No.");
#endif

	if (ui_tree_node(app.ui, "Inputs", false)) {
		static char test_buf[256] = "Hello, world!";

		ui_columns(app.ui, 2, (f32[]) { 0.25f, 0.75f });
		ui_label(app.ui, "Text input: ");
		if (ui_input(app.ui, test_buf, sizeof test_buf)) {
			info("%s", test_buf);
		}

		static f64 test_num = 10.0;

		ui_label(app.ui, "Number input: ");
		if (ui_number_input(app.ui, &test_num)) {
			info("%g", test_num);
		}

		ui_tree_pop(app.ui);
	}

	if (ui_tree_node(app.ui, "Combo Box", false)) {
		static i32 item = 0;

		ui_columns(app.ui, 2, (f32[]) { 0.25f, 0.75f });
		ui_label(app.ui, "Combo Box:");
		ui_combo(app.ui, &item, ((const char*[]) {
			"Item 1",
			"Item 2",
			"Item 3",
			"Item 4",
			"Item 5",
			"Item 6"}), 6);

		ui_tree_pop(app.ui);
	}

	if (ui_tree_node(app.ui, "Knob", false)) {
		static f32 test = 25.0f;
		static f32 test2 = 50.0f;

		char text[256];

		ui_columns(app.ui, 2, (f32[]) { 0.5f, 0.5f });

		ui_knob(app.ui, &test, 0.0f, 100.0f);
		ui_knob_ex(app.ui, "red_knob", &test2, 0.0f, 100.0f);

		sprintf(text, "%.2f", test);
		ui_label_ex(app.ui, "knob_label", text);
		sprintf(text, "%.2f", test2);
		ui_label_ex(app.ui, "knob_label", text);

		ui_tree_pop(app.ui);
	}

	if (ui_tree_node(app.ui, "Colour Picker", false)) {
		ui_columns(app.ui, 2, (f32[]) { 0.25f, 0.75f });
		ui_label(app.ui, "Colour Picker:");

		static v4f colour = { 0.0f, 0.2f, 0.4f, 1.0f };
		ui_colour_picker(app.ui, &colour);

		ui_label(app.ui, "Colour Picker:");
		
		static v4f colour2 = { 0.4f, 0.1f, 0.0f, 1.0f };
		ui_colour_picker(app.ui, &colour2);

		ui_tree_pop(app.ui);
	}

	if (ui_tree_node(app.ui, "Hierarchy", false)) {
		for (usize i = 0; i < 10; i++) {
			if (ui_tree_node(app.ui, "Node", false)) {
				for (usize ii = 0; ii < 10; ii++) {
					if (ui_tree_node(app.ui, "Child", false)) {
						for (usize iii = 0; iii < 10; iii++) {
							ui_tree_node(app.ui, "Leaf", true);
						}

						ui_tree_pop(app.ui);
					}
				}

				ui_tree_pop(app.ui);
			}
		}

		ui_tree_pop(app.ui);
	}

	ui_end_container(app.ui);

	ui_end(app.ui);

	video.begin_framebuffer(video.get_default_fb());
		ui_draw(app.ui);
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_ui(app.ui);
	ui_deinit();
}
