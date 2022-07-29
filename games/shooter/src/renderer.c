#include "renderer.h"

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
		(struct pipeline_attributes) {
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
			.count = 2,
			.stride = sizeof(struct mesh_vertex)
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
						{
							.name     = "Transforms",
							.binding  = 2,
							.stage    = pipeline_stage_vertex,
							.resource = {
								.type         = pipeline_resource_uniform_buffer,
								 /* TODO: Make this dynamic. Add dynamic UBs? */
								.uniform.size = sizeof(m4f) * 1000
							}
						},
					},
					.count = 3,
				}
			},
			.count = 1
		}
	);

	return renderer;
}

void free_renderer(struct renderer* renderer) {
	video.free_pipeline(renderer->pipeline);

	free_table(renderer->drawlist);

	free_vector(renderer->transforms);
	core_free(renderer);
}

void renderer_begin(struct renderer* renderer) {
	vector_clear(renderer->transforms);

	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		usize* count = table_get(renderer->drawlist, *i);
		*count = 0;
	}
}

void renderer_end(struct renderer* renderer) {

}

void renderer_push(struct renderer* renderer, struct mesh* mesh, m4f transform) {
	usize* count = table_get(renderer->drawlist, mesh);
	if (!count) {
		table_set(renderer->drawlist, mesh, 0);
		count = table_get(renderer->drawlist, mesh);
	}

	*count += 1;

	vector_push(renderer->transforms, transform);
	renderer->ub_transforms[vector_count(renderer->transforms) - 1] = transform;
}

void renderer_finalise(struct renderer* renderer, struct camera* camera) {
	renderer->vertex_config.view = get_camera_view(camera);
	renderer->vertex_config.projection = get_camera_projection(camera);

	renderer->fragment_config.camera_pos = camera->position;

	video.update_pipeline_uniform(renderer->pipeline, "primary", "VertexConfig",   &renderer->vertex_config);
	video.update_pipeline_uniform(renderer->pipeline, "primary", "FragmentConfig", &renderer->fragment_config);
	video.update_pipeline_uniform(renderer->pipeline, "primary", "Transforms",     renderer->ub_transforms);

	video.begin_pipeline(renderer->pipeline);
		for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
			usize count = *(usize*)table_get(renderer->drawlist, *i);
			struct mesh* mesh = *i;

			video.bind_vertex_buffer(mesh->vb);
			video.bind_index_buffer(mesh->ib);
			video.bind_pipeline_descriptor_set(renderer->pipeline, "primary", 0);
			video.draw_indexed(mesh->count, 0, count);
		}
	video.end_pipeline(renderer->pipeline);
}
