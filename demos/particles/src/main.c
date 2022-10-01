#include <stdio.h>
#include <time.h>

#include <corrosion/cr.h>

#define dispatch_size 256
#define max_particles dispatch_size * 1024

struct particle {
	v2f pos;
	v2f vel;
	v4f colour;
};

struct particle_info {
	v4f colour;
};

struct {
	struct ui* ui;

	struct pipeline* draw_pipeline;
	struct pipeline* update_pipeline;

	struct storage* particles;

	f64 fps_buf_up_time;
	char fps_buf[256];

	struct {
		m4f camera;
	} vertex_config;

	struct {
		u32 particle_count;
		f32 ts;
		v2f window_size;
	} particles_config;
} app;

struct app_config cr_config() {
	return (struct app_config) {
		.name = "Compute Particles",
		.video_config = (struct video_config) {
			.api = video_api_vulkan,
#ifdef debug
			.enable_validation = true,
#else
			.enable_validation = false,
#endif
			.clear_colour = make_rgba(0x000000, 255),
			.enable_vsync = true
		},
		.window_config = (struct window_config) {
			.title = "Compute Particles",
			.size = make_v2i(1920, 1080),
			.resizable = true
		}
	};
}

static f32 rand_flt() {
	return (f32)rand() / (f32)RAND_MAX;
}

void cr_init() {
	ui_init();

	app.ui = new_ui(video.get_default_fb());

	const struct shader* update_shader   = load_shader("shaders/update.csh");
	const struct shader* particle_shader = load_shader("shaders/particle.csh");

	struct particle* initial_particles = core_calloc(max_particles, sizeof *initial_particles);

	v2i ws = get_window_size();

	for (usize i = 0; i < max_particles; i++) {
		initial_particles[i].pos = make_v2f((f32)ws.x * 0.5f, (f32)ws.y * 0.5f);
		initial_particles[i].vel = make_v2f(map(rand_flt(), 0.0f, 1.0f, -1000.0f, 1000.0f), map(rand_flt(), 0.0f, 1.0f, -1000.0f, 1000.0f));
		initial_particles[i].colour = make_v4f(rand_flt(), rand_flt(), rand_flt(), rand_flt());
	}

	app.particles = video.new_storage(storage_flags_vertex_buffer, max_particles * sizeof(struct particle),      initial_particles);

	core_free(initial_particles);

	app.draw_pipeline = video.new_pipeline(
		pipeline_flags_draw_points,
		particle_shader,
		video.get_default_fb(),
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name = "position",
								.location = 0,
								.offset = offsetof(struct particle, pos),
								.type = pipeline_attribute_vec2
							},
							{
								.name = "colour",
								.location = 1,
								.offset = offsetof(struct particle, colour),
								.type = pipeline_attribute_vec4
							}
						},
						.count = 2
					},
					.stride = sizeof(struct particle),
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

							.name = "Config",
							.binding = 0,
							.stage = pipeline_stage_vertex,
							.resource = {
								.type = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof app.vertex_config
							}
						}
					},
					.count = 1,
					.name = "primary"
				}
			},
			.count = 1
		});
	
	app.update_pipeline = video.new_compute_pipeline(pipeline_flags_none, update_shader,
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
								.uniform.size = sizeof app.particles_config
							}
						},
						{
							.name = "Buffer",
							.binding = 1,
							.stage = pipeline_stage_compute,
							.resource = {
								.type = pipeline_resource_storage,
								.storage = app.particles
							}
						}
					},
					.count = 2,
				}
			},
			.count = 1
		});
}

void cr_update(f64 ts) {
	ui_begin(app.ui);

	app.fps_buf_up_time += ts;

	if (app.fps_buf_up_time >= 1.0) {
		app.fps_buf_up_time = 0.0;
		sprintf(app.fps_buf, "FPS: %g", 1.0 / ts);
	}

	ui_begin_container(app.ui, make_v4f(0.0f, 0.0f, 0.1f, 0.1f), true);
	ui_label(app.ui, app.fps_buf);
	ui_end_container(app.ui);

	ui_end(app.ui);

	v2i ws = get_window_size();

	app.vertex_config.camera = m4f_ortho(0.0f, (f32)ws.x, (f32)ws.y, 0.0f, -1.0f, 1.0f);
	app.particles_config.ts = (f32)ts;
	app.particles_config.particle_count = max_particles;
	app.particles_config.window_size = make_v2f((f32)ws.x, (f32)ws.y);

	video.update_pipeline_uniform(app.draw_pipeline,   "primary", "Config", &app.vertex_config);
	video.update_pipeline_uniform(app.update_pipeline, "primary", "config", &app.particles_config);

	video.begin_pipeline(app.update_pipeline);
		video.bind_pipeline_descriptor_set(app.update_pipeline, "primary", 0);

		video.storage_barrier(app.particles, storage_state_compute_read_write);

		video.invoke_compute(make_v3u(dispatch_size, 1, 1));

		video.storage_barrier(app.particles, storage_state_vertex_read);
	video.end_pipeline(app.update_pipeline);

	video.begin_framebuffer(video.get_default_fb());
		video.begin_pipeline(app.draw_pipeline);
			video.bind_pipeline_descriptor_set(app.draw_pipeline, "primary", 0);

			video.storage_bind_as(app.particles, storage_bind_as_vertex_buffer, 0);

			video.draw(max_particles, 0, 1);
		video.end_pipeline(app.draw_pipeline);

		ui_draw(app.ui);
		gizmos_draw();
	video.end_framebuffer(video.get_default_fb());
}

void cr_deinit() {
	video.free_pipeline(app.draw_pipeline);
	video.free_pipeline(app.update_pipeline);
	video.free_storage(app.particles);

	free_ui(app.ui);

	ui_deinit();
}
