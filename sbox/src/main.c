#include <stdio.h>

#include <corrosion/cr.h>

//#if 0

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

	struct shader* compute_shader;
	struct pipeline* compute_pipeline;

	struct storage* com_in;
	struct storage* com_out;

	struct {
		u32 number_count;
	} compute_config;
	f32 numbers[256];
} app;

void on_resize(const struct window_event* event) {
	info("Window resized: %d, %d", event->size_changed.new_size.x, event->size_changed.new_size.y);
}

void on_event(const struct window_event* event) {
	info("Window event with type: %d", event->type);
}

void on_type(const struct window_event* event) {
	info("Text typed: %.*s", event->text_typed.len, event->text_typed.text);
}

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Sandbox",
		.video_config = (struct video_config) {
			.api = video_api_vulkan,
#ifdef debug
			.enable_validation = true,
#else
			.enable_validation = false,
#endif
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

	window_event_subscribe(window_event_size_changed, on_resize);
	window_event_subscribe(window_event_any, on_event);
	window_event_subscribe(window_event_text_typed, on_type);

	app.r = 0.0f;
	app.fps_timer = 1.0;

	app.texturea = load_texture("res/chad.jpg", texture_flags_filter_linear);
	app.textureb = load_texture("res/test.png", texture_flags_filter_linear);

	for (usize i = 0; i < 256; i++) {
		app.numbers[i] = 40.0f;
	}

	app.dejavusans = load_font("res/DejaVuSans.ttf", 24.0f);

	app.ui = new_ui(video.get_default_fb());

	struct ui_stylesheet* stylesheet = res_load("stylesheet", "res/uistyle.dt", null);
	ui_stylesheet(app.ui, stylesheet);

	app.com_in  = video.new_storage(storage_flags_none,         sizeof app.numbers, app.numbers);
	app.com_out = video.new_storage(storage_flags_cpu_readable, sizeof app.numbers, null);

	app.compute_shader = load_shader("shaders/computetest.csh");
	app.compute_pipeline = video.new_compute_pipeline(pipeline_flags_none, app.compute_shader,
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name     = "config",
							.binding  = 0,
							.stage    = pipeline_stage_compute,
							.resource = {
								.type         = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.compute_config
							}
						},
						{
							.name  = "in_buf",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.com_in
							}
						},
						{
							.name  = "out_buf",
							.binding = 2,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.com_out
							}
						},
					},
					.count = 3,
				}
			},
			.count = 1
		});
	
	app.compute_config.number_count = 256;
	memset(app.numbers, 0, sizeof app.numbers);

	const struct shader* invert_shader = load_shader("shaders/invert.csh");

	app.fb = video.new_framebuffer(framebuffer_flags_headless | framebuffer_flags_fit, get_window_size(),
		(struct framebuffer_attachment_desc[]) {
			{
				.type   = framebuffer_attachment_colour,
				.format = framebuffer_format_rgba16f,
				.clear_colour = make_rgba(0x010111, 255)
			}
		}, 1);

	v2f tri_verts[] = {
		/* Position        UV */
		{ -1.0,   1.0 },   { 0.0f, 0.0f },
		{ -1.0,  -3.0 },   { 0.0f, 2.0f },
		{  3.0,   1.0 },   { 2.0f, 0.0f }
	};

	app.tri_vb = video.new_vertex_buffer(tri_verts, sizeof tri_verts, vertex_buffer_flags_none);

	app.invert_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		invert_shader,
		video.get_default_fb(),
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
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
						.count = 2
					},
					.stride = sizeof(v2f) * 2,
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = 0
				}
			},
			.count = 1
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
								.type    = pipeline_resource_texture,
								.texture = video.get_attachment(app.fb, 0)
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

	video.update_pipeline_uniform(app.compute_pipeline, "primary", "config", &app.compute_config);

	video.begin_pipeline(app.compute_pipeline);
		video.bind_pipeline_descriptor_set(app.compute_pipeline, "primary", 0);

		video.storage_barrier(app.com_out, storage_state_compute_write);

		video.invoke_compute(make_v3u(1, 1, 1));

		video.storage_barrier(app.com_out, storage_state_vertex_read);
	video.end_pipeline(app.compute_pipeline);

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
			video.bind_vertex_buffer(app.tri_vb, 0);
			video.bind_pipeline_descriptor_set(app.invert_pip, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.invert_pip);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	free_ui(app.ui);

	video.free_storage(app.com_in);
	video.free_storage(app.com_out);

	video.free_framebuffer(app.fb);
	video.free_pipeline(app.invert_pip);
	video.free_pipeline(app.compute_pipeline);
	video.free_vertex_buffer(app.tri_vb);

	free_simple_renderer(app.renderer);

	ui_deinit();
}

//#endif

#if 0
struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct pipeline* pipeline;
	struct vertex_buffer* tri_vb;
	struct index_buffer* tri_ib;

	struct texture* texture;

	struct {
		m4f transform;
	} v_config;

	struct {
		v3f colour;
	} f_config;

	struct {
		v3f colour;
	} f_config_blue;

	f64 time;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Sandbox",
		.video_config = (struct video_config) {
			.api = video_api_opengl,
#ifdef debug
			.enable_validation = true,
#else
			.enable_validation = false,
#endif
			.clear_colour = make_rgba(0x000000, 255)
		},
		.window_config = (struct window_config) {
			.title = "Sandbox",
			.size = make_v2i(1366, 768),
			.resizable = true
		}
	};
}

void cr_init() {
	const struct shader* shader = load_shader("shaders/test.csh");

	app.texture = load_texture("res/chad.jpg", texture_flags_filter_linear);

	app.pipeline = video.new_pipeline(
		pipeline_flags_draw_tris | pipeline_flags_cull_back_face,
		shader,
		video.get_default_fb(),
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name = "position",
								.location = 0,
								.offset = offsetof(struct vertex, position),
								.type = pipeline_attribute_vec2
							},
							{
								.name = "uv",
								.location = 1,
								.offset = offsetof(struct vertex, uv),
								.type = pipeline_attribute_vec2
							}
						},
						.count = 2
					},
					.stride = sizeof(struct vertex),
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = 0
				}
			},
			.count = 1
		},
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "image",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = app.texture
							}
						},
					},
					.count = 1,
					.name = "primary"
				},
				{
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "VertexConfig",
							.binding = 0,
							.stage = pipeline_stage_vertex,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.v_config
							}
						}
					},
					.count = 1,
					.name = "ubdata"

				},
				{
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "FragmentConfig",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.f_config
							}
						}
					},
					.count = 1,
					.name = "fubdata"

				},
				{
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "FragmentConfig",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.f_config
							}
						}
					},
					.count = 1,
					.name = "fubdata_blue"

				}
			},
			.count = 4
		}
	);

	struct vertex verts[] = {
		{ { -0.5f,  0.5f }, { 1.0f, 0.0f } },
		{ { -0.5f, -0.5f }, { 1.0f, 1.0f } },
		{ {  0.5f, -0.5f }, { 0.0f, 1.0f } },
		{ {  0.5f,  0.5f }, { 0.0f, 0.0f } }
	};

	u16 indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	app.tri_vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);
	app.tri_ib = video.new_index_buffer(indices, sizeof indices, index_buffer_flags_u16);
}

void cr_update(f64 ts) {
	app.time += ts;

	app.v_config.transform = m4f_rotation(euler(make_v3f(0.0f, 0.0f, (f32)app.time * 10.0f)));
	app.f_config.colour = make_rgb(0xff0000);
	app.f_config_blue.colour = make_rgb(0x0000ff);

	gizmo_camera(&(struct camera) {
		.fov = 70.0f,
		.near_plane = 0.1f,
		.far_plane = 100.0f
	});

	gizmo_colour(make_rgba(0x00ffff, 255));
	gizmo_box(make_v3f(1.0f, -1.0f, 3.0f), make_v3f(1.0f, 1.0f, 1.0f), euler(make_v3f(0.0f, 25.0f, 0.0f)));

	video.update_pipeline_uniform(app.pipeline, "ubdata", "VertexConfig", &app.v_config);
	video.update_pipeline_uniform(app.pipeline, "fubdata", "FragmentConfig", &app.f_config);
	video.update_pipeline_uniform(app.pipeline, "fubdata_blue", "FragmentConfig", &app.f_config_blue);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.pipeline);
			video.bind_pipeline_descriptor_set(app.pipeline, "primary", 0);
			video.bind_pipeline_descriptor_set(app.pipeline, "ubdata", 1);

			if (key_pressed(key_space)) {
				video.bind_pipeline_descriptor_set(app.pipeline, "fubdata_blue", 2);
			} else {
				video.bind_pipeline_descriptor_set(app.pipeline, "fubdata", 2);
			}

			video.bind_vertex_buffer(app.tri_vb, 0);
			video.bind_index_buffer(app.tri_ib);
			video.draw_indexed(6, 0, 1);
		video.end_pipeline(app.pipeline);

		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.pipeline);
	video.free_vertex_buffer(app.tri_vb);
	video.free_index_buffer(app.tri_ib);
}
#endif
