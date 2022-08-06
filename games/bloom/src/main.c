#include <stdio.h>
#include <time.h>

#include <corrosion/cr.h>

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct ui* ui;

	struct texture* image;

	struct pipeline* pipeline;

	struct vertex_buffer* vb;
	struct index_buffer* ib;

	struct {
		v2f image_size;
	} bloom_config;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Shooter",
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
			.title = "Bloom",
			.size = make_v2i(1920, 1080),
			.resizable = true
		}
	};
}

void cr_init() {
	ui_init();

	app.image = load_texture("textures/testimage.png", texture_flags_filter_linear);

	const struct shader* shader = load_shader("shaders/bloom.csh");

	app.pipeline = video.new_pipeline(
		pipeline_flags_draw_tris,
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
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "config",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform = {
									.size = sizeof app.bloom_config
								}
							}
						},
						{
							.name = "image",
							.binding = 1,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = app.image
							}
						}
					},
					.count = 2
				}
			},
			.count = 1
		}
	);

	struct vertex verts[] = {
		{ { -0.5f,  0.5f }, { 0.0f, 1.0f } },
		{ {  0.5f,  0.5f }, { 1.0f, 1.0f } },
		{ {  0.5f, -0.5f }, { 1.0f, 0.0f } },
		{ { -0.5f, -0.5f }, { 0.0f, 0.0f } }
	};

	u16 indices[] = {
		3, 2, 1,
		3, 1, 0
	};

	app.vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);
	app.ib = video.new_index_buffer(indices, 6, index_buffer_flags_u16);

	app.ui = new_ui(video.get_default_fb());
}

void cr_update(f64 ts) {
	ui_begin(app.ui);

	ui_label(app.ui, "Hello, world");

	ui_end(app.ui);

	video.update_pipeline_uniform(app.pipeline, "primary", "config", &app.bloom_config);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.pipeline);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_index_buffer(app.ib);
			video.bind_pipeline_descriptor_set(app.pipeline, "primary", 0);
			video.draw_indexed(6, 0, 1);
		video.end_pipeline(app.pipeline);
		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_vertex_buffer(app.vb);
	video.free_index_buffer(app.ib);
	video.free_pipeline(app.pipeline);

	free_ui(app.ui);

	ui_deinit();
}
