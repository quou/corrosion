#include <corrosion/cr.h>

#define sim_size make_v2i(400, 250)
#define pixel_size 4

struct vertex {
	v2f position;
	v2f uv;
};

struct {
	struct texture* result;
	struct storage* particles;
	struct storage* temp_particles;

	struct pipeline* process_pip;
	struct pipeline* draw_pip;

	struct vertex_buffer* vb;
	struct index_buffer* ib;

	struct {
		v2i size;
	} config;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Sand Simulation",
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
			.title = "Sand Simulation",
			.size = v2i_scale(sim_size, pixel_size),
			.resizable = false
		}
	};
}

void cr_init() {
	const struct shader* draw_shader = load_shader("shaders/quad.csh");
	const struct shader* process_shader = load_shader("shaders/process.csh");

	struct image i = { .size = sim_size };
	app.result = video.new_texture(&i, texture_flags_filter_none | texture_flags_storage, texture_format_rgba8i);
	app.particles = video.new_storage(storage_flags_cpu_writable, sim_size.x * sim_size.y * sizeof(i32), null);
	app.temp_particles = video.new_storage(storage_flags_none, sim_size.x * sim_size.y * sizeof(i32), null);

	app.process_pip = video.new_compute_pipeline(pipeline_flags_none, process_shader,
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
							.name = "particles",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.particles
							}
						},
						{
							.name = "temp_particles",
							.binding = 2,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.temp_particles
							}
						},
						{
							.name = "result",
							.binding = 3,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_texture_storage,
								.texture = app.result
							}
						}
					},
					.count = 4
				}
			},
			.count = 1
		});
	
	app.draw_pip = video.new_pipeline(
		pipeline_flags_draw_tris,
		draw_shader,
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
						}
					},
					.count = 1
				}
			},
			.count = 1
		}
	);


	struct vertex verts[] = {
		{ { -1.0f, -1.0f }, { 0.0f, 1.0f } },
		{ {  1.0f, -1.0f }, { 1.0f, 1.0f } },
		{ {  1.0f,  1.0f }, { 1.0f, 0.0f } },
		{ { -1.0f,  1.0f }, { 0.0f, 0.0f } }
	};

	u16 indices[] = {
		3, 2, 1,
		3, 1, 0
	};

	app.vb = video.new_vertex_buffer(verts, sizeof verts, vertex_buffer_flags_none);
	app.ib = video.new_index_buffer(indices, 6, index_buffer_flags_u16);
}

void cr_update() {
	app.config.size = sim_size;

	if (mouse_btn_pressed(mouse_btn_left)) {
		v2i mp = get_mouse_pos();

		v2i mapped_mp = v2i_div(mp, make_v2i(pixel_size, pixel_size));
		if (mp.x >= 0 && mp.y >= 0 && mapped_mp.x < app.config.size.x && mapped_mp.y < app.config.size.y) {
			i32 val = 1;

			i32 offset = mapped_mp.x + mapped_mp.y * app.config.size.x;

			video.update_storage_region(app.particles, &val, sizeof val * (usize)offset, sizeof val);
		}
	} else if (mouse_btn_pressed(mouse_btn_right)) {
		v2i mp = get_mouse_pos();

		v2i mapped_mp = v2i_div(mp, make_v2i(pixel_size, pixel_size));
		if (mp.x >= 0 && mp.y >= 0 && mapped_mp.x < app.config.size.x && mapped_mp.y < app.config.size.y) {
			i32 val = 2;

			i32 offset = mapped_mp.x + mapped_mp.y * app.config.size.x;

			video.update_storage_region(app.particles, &val, sizeof val * (usize)offset, sizeof val);
		}
	}

	video.copy_storage(app.temp_particles, 0, app.particles, 0, sim_size.x * sim_size.y * sizeof(i32));

	video.update_pipeline_uniform(app.process_pip, "primary", "config", &app.config);

	video.texture_barrier(app.result, texture_state_shader_write);

	video.begin_pipeline(app.process_pip);
		video.bind_pipeline_descriptor_set(app.process_pip, "primary", 0);
		video.invoke_compute(make_v3u((u32)app.config.size.x / 16, (u32)app.config.size.y + 1 / 16, 1));
	video.end_pipeline(app.process_pip);

	video.texture_barrier(app.result, texture_state_shader_graphics_read);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.draw_pip);
			video.bind_vertex_buffer(app.vb, 0);
			video.bind_index_buffer(app.ib);
			video.bind_pipeline_descriptor_set(app.draw_pip, "primary", 0);
			video.draw_indexed(6, 0, 1);
		video.end_pipeline(app.draw_pip);
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.draw_pip);
	video.free_pipeline(app.process_pip);
	
	video.free_vertex_buffer(app.vb);
	video.free_index_buffer(app.ib);

	video.free_storage(app.particles);
	video.free_storage(app.temp_particles);
	video.free_texture(app.result);
}
