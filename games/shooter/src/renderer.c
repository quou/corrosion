#include "renderer.h"

#define renderer_vert_buffer_bind_point 0
#define renderer_inst_buffer_bind_point 1

static void mesh_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	init_model_from_fbx(payload, raw, raw_size);
}

static void mesh_on_unload(void* payload, usize payload_size) {
	deinit_model(payload);
}

void register_renderer_resources() {
	reg_res_type("model", &(struct res_config) {
		.payload_size = sizeof(struct model),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.on_load = mesh_on_load,
		.on_unload = mesh_on_unload
	});
}

struct model* load_model(const char* filename) {
	return res_load("model", filename, null);
}

struct renderer* new_renderer(const struct framebuffer* framebuffer) {
	struct renderer* renderer = core_calloc(1, sizeof *renderer);
	renderer->framebuffer = framebuffer;

	const struct shader* shader = load_shader("shaders/lit.csh");

	renderer->pipeline = video.new_pipeline(
		pipeline_flags_depth_test | pipeline_flags_draw_tris,
		shader,
		renderer->framebuffer,
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name     = "position",
								.location = 0,
								.offset   = offsetof(struct mesh_vertex, position),
								.type     = pipeline_attribute_vec3
							},
							{
								.name     = "normal",
								.location = 1,
								.offset   = offsetof(struct mesh_vertex, normal),
								.type     = pipeline_attribute_vec3
							},
						},
						.count = 2
					},
					.stride = sizeof(struct mesh_vertex),
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = renderer_vert_buffer_bind_point
				},
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name = "r0",
								.location = 2,
								.offset = offsetof(struct mesh_instance_data, r0),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r1",
								.location = 3,
								.offset = offsetof(struct mesh_instance_data, r1),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r2",
								.location = 4,
								.offset = offsetof(struct mesh_instance_data, r2),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r3",
								.location = 5,
								.offset = offsetof(struct mesh_instance_data, r3),
								.type = pipeline_attribute_vec4
							},
						},
						.count = 4,
					},
					.stride = sizeof(struct mesh_instance_data),
					.rate = pipeline_attribute_rate_per_instance,
					.binding = renderer_inst_buffer_bind_point
				}
			},
			.count = 2
		},
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name     = "VertexConfig",
							.binding  = 0,
							.stage    = pipeline_stage_vertex,
							.resource = {
								.type         = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof renderer->vertex_config
							}
						},
						{
							.name     = "FragmentConfig",
							.binding  = 1,
							.stage    = pipeline_stage_fragment,
							.resource = {
								.type         = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof renderer->fragment_config
							}
						},
					},
					.count = 2,
				}
			},
			.count = 1
		}
	);

	return renderer;
}

void free_renderer(struct renderer* renderer) {
	video.free_pipeline(renderer->pipeline);

	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		struct mesh_instance* instance = table_get(renderer->drawlist, *i);

		video.free_vertex_buffer(instance->data);
	}
	free_table(renderer->drawlist);

	core_free(renderer);
}

void renderer_begin(struct renderer* renderer) {
	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		struct mesh_instance* instance = table_get(renderer->drawlist, *i);
		instance->count = 0;
	}
}

void renderer_end(struct renderer* renderer) {

}

void renderer_push(struct renderer* renderer, struct mesh* mesh, m4f transform) {
	struct mesh_instance* instance = table_get(renderer->drawlist, mesh);
	if (!instance) {
		table_set(renderer->drawlist, mesh, (struct mesh_instance) { 0 });
		instance = table_get(renderer->drawlist, mesh);

		/* TODO: Dynamically-sized vertex buffer? */
		instance->data = video.new_vertex_buffer(null, sizeof(struct mesh_instance_data) * 1000llu, vertex_buffer_flags_dynamic);
	}

	struct mesh_instance_data data = {
		.r0 = make_v4f(transform.m[0][0], transform.m[0][1], transform.m[0][2], transform.m[0][3]),
		.r1 = make_v4f(transform.m[1][0], transform.m[1][1], transform.m[1][2], transform.m[1][3]),
		.r2 = make_v4f(transform.m[2][0], transform.m[2][1], transform.m[2][2], transform.m[2][3]),
		.r3 = make_v4f(transform.m[3][0], transform.m[3][1], transform.m[3][2], transform.m[3][3])
	};

	video.update_vertex_buffer(instance->data, &data, sizeof data, instance->count * sizeof data);

	instance->count++;
}

void renderer_finalise(struct renderer* renderer, struct camera* camera) {
	renderer->vertex_config.view = get_camera_view(camera);
	renderer->vertex_config.projection = get_camera_projection(camera);

	renderer->fragment_config.camera_pos = camera->position;

	video.update_pipeline_uniform(renderer->pipeline, "primary", "VertexConfig",   &renderer->vertex_config);
	video.update_pipeline_uniform(renderer->pipeline, "primary", "FragmentConfig", &renderer->fragment_config);

	video.begin_pipeline(renderer->pipeline);
		for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
			struct mesh_instance* instance = table_get(renderer->drawlist, *i);
			struct mesh* mesh = *i;

			video.bind_vertex_buffer(mesh->vb,       renderer_vert_buffer_bind_point);
			video.bind_vertex_buffer(instance->data, renderer_inst_buffer_bind_point);
			video.bind_index_buffer(mesh->ib);
			video.bind_pipeline_descriptor_set(renderer->pipeline, "primary", 0);
			video.draw_indexed(mesh->count, 0, instance->count);
		}
	video.end_pipeline(renderer->pipeline);
}
