#include <corrosion/cr.h>

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct texture* result;

	struct pipeline* process_pip;
	struct pipeline* draw_pip;

	struct vertex_buffer* vb;

	struct camera camera;

	struct {
		v2i size;
	} config;

	struct {
		v2f resolution;
		f32 fov;
		pad(4); v3f camera_position;
		pad(4); m4f camera;
	} render_data;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Voxel Engine",
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
			.title = "Voxel Engine",
			.size = make_v2i(1920, 1080),
			.resizable = false
		}
	};
}

void cr_init() {
	app.camera = (struct camera) {
		.fov = 70.0f,
		.position = make_v3f(3.0f, 0.0f, 3.0f),
		.rotation = make_v3f(0.0f, -40.0f, 0.0f)
	};

	const struct shader* raymarch_shader = load_shader("shaders/raymarch.csh");
	const struct shader* sdf_gen_shader = load_shader("shaders/sdf_gen.csh");

	struct image i = { .size = get_window_size() };
	app.result = video.new_texture(&i, texture_flags_filter_none | texture_flags_storage);

	app.process_pip = video.new_compute_pipeline(pipeline_flags_none, sdf_gen_shader,
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name = "config",
							.binding = 0,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.config
							}
						},
						{
							.name = "result",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_texture_storage,
								.texture = app.result
							}
						}
					},
					.count = 2
				}
			},
			.count = 1
		});
	
	app.draw_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		raymarch_shader,
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
							.name = "image",
							.binding = 0,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = app.result
							}
						},
						{
							.name = "RenderData",
							.binding = 1,
							.stage = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.render_data
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
		{ { -1.0,   1.0 }, { 0.0f, 0.0f } },
		{ { -1.0,  -3.0 }, { 0.0f, 2.0f } },
		{ {  3.0,   1.0 }, { 2.0f, 0.0f } }
	};

	app.vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);
}

void cr_update() {
	app.config.size = get_window_size();

	app.render_data.fov = to_rad(app.camera.fov);
	app.render_data.resolution = make_v2f(app.config.size.x, app.config.size.y);
	app.render_data.camera_position = app.camera.position;
	app.render_data.camera = get_camera_view(&app.camera);

	video.update_pipeline_uniform(app.process_pip, "primary", "config", &app.config);
	video.update_pipeline_uniform(app.draw_pip, "primary", "RenderData", &app.render_data);

	video.texture_barrier(app.result, texture_state_shader_write);

	video.begin_pipeline(app.process_pip);
		video.bind_pipeline_descriptor_set(app.process_pip, "primary", 0);
		video.invoke_compute(make_v3u((u32)app.config.size.x / 16, (u32)app.config.size.y + 1 / 16, 1));
	video.end_pipeline(app.process_pip);

	video.texture_barrier(app.result, texture_state_shader_graphics_read);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.draw_pip);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_pipeline_descriptor_set(app.draw_pip, "primary", 0);
			video.draw(3, 0, 1);
		video.end_pipeline(app.draw_pip);
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.draw_pip);
	video.free_pipeline(app.process_pip);
	
	video.free_vertex_buffer(app.vb);

	video.free_texture(app.result);
}
