#include <stdio.h>

#include <corrosion/cr.h>

struct {
	struct texture* texturea;
	struct texture* textureb;

	struct framebuffer* fb;
	struct pipeline* invert_pip;

	struct vertex_buffer* tri_vb;

	struct simple_renderer* renderer;

	struct font* dejavusans;

	struct ui* ui;

	f32 r;

	f64 fps_timer;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Sandbox",
		.video_config = (struct video_config) {
			.api = video_api_vulkan,
			.enable_validation = true,
			.clear_colour = make_rgba(0x000000, 255)
		},
		.window_config = (struct window_config) {
			.title = "Sandbox",
			.size = make_v2i(800, 600),
			.resizable = true
		}
	};
}

void cr_init() {
	ui_init();

	app.r = 0.0f;
	app.fps_timer = 1.0;

	app.texturea = load_texture("res/chad.jpg", texture_flags_filter_linear);
	app.textureb = load_texture("res/test.png", texture_flags_filter_linear);

	app.dejavusans = load_font("res/DejaVuSans.ttf", 24.0f);

	app.ui = new_ui(video.get_default_fb());

	struct ui_stylesheet* stylesheet = res_load("stylesheet", "res/uistyle.dt", null);
	ui_stylesheet(app.ui, stylesheet);

	const struct shader* invert_shader = load_shader("shaderbin/invert.csh");

	app.fb = video.new_framebuffer(framebuffer_flags_headless | framebuffer_flags_fit, get_window_size(),
		(struct framebuffer_attachment_desc[]) {
			{
				.type   = framebuffer_attachment_colour,
				.format = framebuffer_format_rgba16f,
				.clear_colour = make_rgba(0x010111, 255)
			}
		}, 1);

	v2f tri_verts[] = {
		/* Position          UV */
		{ -1.0, -1.0 },      { 0.0f, 0.0f },
		{ -1.0,  3.0 },      { 0.0f, 2.0f },
		{  3.0, -1.0 },      { 2.0f, 0.0f }
	};

	app.tri_vb = video.new_vertex_buffer(tri_verts, sizeof tri_verts, vertex_buffer_flags_none);

	app.invert_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		invert_shader,
		video.get_default_fb(),
		(struct pipeline_attributes) {
			.attributes = (struct pipeline_attribute[]) {
				{
					.name     = "position",
					.location = 0,
					.offset   = 0,
					.type     = pipeline_attribute_vec2
				},
				{
					.name     = "uv",
					.location = 1,
					.offset   = sizeof(v2f),
					.type     = pipeline_attribute_vec2
				},
			},
			.count = 2,
			.stride = sizeof(v2f) * 2
		},
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name     = "src",
							.binding  = 0,
							.stage    = pipeline_stage_fragment,
							.resource = {
								.type        = pipeline_resource_framebuffer,
								.framebuffer = {
									.ptr        = app.fb,
									.attachment = 0
								}
							}
						},
					},
					.count = 1,
				}
			},
			.count = 1
		}
	);

	app.renderer = new_simple_renderer(app.fb);

	/*write_pak("test.pak", (const char* []) {
		"res/uistyle.dt",
		"res/chad.jpg",
		"res/DejaVuSans.ttf",
		"res/test.png",
		"shaderbin/invert.csh",
		"shaderbin/line.csh",
		"shaderbin/simple.csh",
		"shaderbin/ui.csh"
	}, 8);*/
}

void cr_update(f64 ts) {
	v2i window_size = get_window_size();

	ui_begin(app.ui);

	ui_columns(app.ui, 1, (f32[]) { 1.0f });

	static char fps_buf[32];
	static char mem_buf[128];
	sprintf(mem_buf, "Memory Usage (KIB): %g", (f64)core_get_memory_usage() / 1024.0);

	app.fps_timer += ts;
	if (app.fps_timer >= 1.0) {
		app.fps_timer = 0.0;
		sprintf(fps_buf, "FPS: %g", 1.0 / ts);
	}
	ui_label(app.ui, fps_buf);
	ui_label(app.ui, mem_buf);

	ui_begin_container(app.ui, make_v4f(0.5f, 0.0f, 0.5f, 1.0f), true);

	if (ui_tree_node(app.ui, "Layout", false)) {
		ui_columns(app.ui, 2, (f32[]) { 0.5f, 0.5f });

		ui_label(app.ui, "Hello, world.");
		ui_label(app.ui, "Hello, world.");
		ui_label(app.ui, "Hello, world.");
		ui_label(app.ui, "Hello, world.");
		if (ui_button(app.ui, "Button A")) {
			info("Button A pressed.");
		}
		if (ui_button(app.ui, "Button B")) {
			info("Button B pressed.");
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

	gizmo_camera(&(struct camera) {
		.fov = 70.0f,
		.near_plane = 0.1f,
		.far_plane = 100.0f
	});

	gizmo_colour(make_rgba(0x00ffff, 255));
	gizmo_box(make_v3f(1.0f, -1.0f, 3.0f), make_v3f(1.0f, 1.0f, 1.0f), euler(make_v3f(0.0f, 25.0f, 0.0f)));
	gizmo_sphere(make_v3f(0.0f, 0.0f, 0.0f), 1.0f);

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

		static v4f colour = { 255, 255, 0, 255 };
		ui_colour_picker(app.ui, &colour);

		ui_label(app.ui, "Colour Picker:");

		static v4f colour2 = { 0, 90, 255, 255 };
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

	video.begin_framebuffer(app.fb);
		simple_renderer_clip(app.renderer, make_v4i(0, 0, window_size.x, window_size.y));
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(0.0f, 0.0f),
			.dimensions = make_v2f(100.0f, 100.0f),
			.colour     = make_rgba(0xffffff, 255),
			.rect       = { 197.0f, 52.0f, 973.0f, 1096.0f },
			.texture    = app.texturea
		});
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(200.0f, 0.0f),
			.dimensions = make_v2f(100.0f, 100.0f),
			.colour     = make_rgba(0xffffff, 255),
			.rect       = { 0.0f, 0.0f, 100.0f, 100.0f },
			.texture    = app.textureb
		});
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(200.0f, 100.0f),
			.dimensions = make_v2f(200.0f, 100.0f),
			.colour     = make_rgba(0xff0000, 255),
		});

		simple_renderer_push_text(app.renderer, &(struct simple_renderer_text) {
			.position = make_v2f(100.0f, 300.0f),
			.text     = "Hello, world!",
			.colour   = make_rgba(0xffffff, 255),
			.font     = app.dejavusans
		});

		simple_renderer_flush(app.renderer);
		simple_renderer_end_frame(app.renderer);
	video.end_framebuffer(app.fb);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.invert_pip);
			video.bind_vertex_buffer(app.tri_vb);
			video.bind_pipeline_descriptor_set(app.invert_pip, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.invert_pip);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_ui(app.ui);

	video.free_framebuffer(app.fb);
	video.free_pipeline(app.invert_pip);
	video.free_vertex_buffer(app.tri_vb);

	free_simple_renderer(app.renderer);

	ui_deinit();
}
