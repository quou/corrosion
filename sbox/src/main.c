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
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Sandbox",
		.api = video_api_vulkan,
		.enable_validation = true,
		.window_config = (struct window_config) {
			.title = "Sandbox",
			.size = make_v2i(800, 600),
			.resizable = true
		}
	};
}

void cr_init() {
	ui_init();

	app.texturea = load_texture("res/chad.jpg", texture_flags_filter_linear);
	app.textureb = load_texture("res/test.png", texture_flags_filter_linear);

	app.dejavusans = load_font("res/DejaVuSans.ttf", 24.0f);

	app.ui = new_ui(video.get_default_fb());

	struct ui_stylesheet* stylesheet = res_load("stylesheet", "res/uistyle.dt", 0);
	ui_stylesheet(app.ui, stylesheet);

	const struct shader* invert_shader = load_shader("shaderbin/invert.csh");

	app.fb = video.new_framebuffer(framebuffer_flags_headless | framebuffer_flags_fit, get_window_size(),
		(struct framebuffer_attachment_desc[]) {
			{
				.type   = framebuffer_attachment_colour,
				.format = framebuffer_format_rgba16f
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
		pipeline_flags_none,
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
}

void cr_update(f64 ts) {
	v2i window_size = get_window_size();

	ui_begin(app.ui);
	ui_columns(app.ui, 2, (f32[]) { 0.5f, 0.5f });
	ui_begin_container(app.ui, make_v4f(0.5f, 0.0f, 0.5f, 1.0f));
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

	static f32 test = 25.0f;
	static f32 test2 = 50.0f;

	char text[256];

	ui_knob(app.ui, &test, 0.0f, 100.0f);
	ui_knob_ex(app.ui, "align_right red_knob", &test2, 0.0f, 100.0f);

	sprintf(text, "%.2f", test);
	ui_label_ex(app.ui, "knob_label", text);
	sprintf(text, "%.2f", test2);
	ui_label_ex(app.ui, "knob_label align_right", text);

	static char test_buf[256] = "Hello, world!";

	ui_columns(app.ui, 2, (f32[]) { 0.25f, 0.70f });
	ui_label(app.ui, "Text input: ");
	if (ui_input_ex(app.ui, "align_right", test_buf, sizeof test_buf)) {
		info("%s", test_buf);
	}

	ui_end_container(app.ui);
	ui_end(app.ui);

	video.begin_framebuffer(app.fb);
		simple_renderer_clip(app.renderer, make_v4i(0, 0, window_size.x, window_size.y));
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(0.0f, 0.0f),
			.dimentions = make_v2f(100.0f, 100.0f),
			.colour     = make_rgba(0xffffff, 255),
			.rect       = { 197.0f, 52.0f, 973.0f, 1096.0f },
			.texture    = app.texturea
		});
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(200.0f, 0.0f),
			.dimentions = make_v2f(100.0f, 100.0f),
			.colour     = make_rgba(0xffffff, 255),
			.rect       = { 0.0f, 0.0f, 100.0f, 100.0f },
			.texture    = app.textureb
		});
		simple_renderer_push(app.renderer, &(struct simple_renderer_quad) {
			.position   = make_v2f(200.0f, 100.0f),
			.dimentions = make_v2f(200.0f, 100.0f),
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
