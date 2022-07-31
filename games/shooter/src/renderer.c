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

static void create_pipeline(struct renderer* renderer) {
	const struct shader* shader = load_shader("shaders/lit.csh");

	renderer->pipeline = video.new_pipeline(
		pipeline_flags_depth_test | pipeline_flags_draw_tris | pipeline_flags_cull_back_face | pipeline_flags_wo_anti_clockwise,
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
							{
								.name     = "uv",
								.location = 2,
								.offset   = offsetof(struct mesh_vertex, uv),
								.type     = pipeline_attribute_vec2
							}
						},
						.count = 3
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
								.location = 3,
								.offset = offsetof(struct mesh_instance_data, r0),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r1",
								.location = 4,
								.offset = offsetof(struct mesh_instance_data, r1),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r2",
								.location = 5,
								.offset = offsetof(struct mesh_instance_data, r2),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "r3",
								.location = 6,
								.offset = offsetof(struct mesh_instance_data, r3),
								.type = pipeline_attribute_vec4
							},
							{
								.name = "diffuse",
								.location = 7,
								.offset = offsetof(struct mesh_instance_data, diffuse),
								.type = pipeline_attribute_vec3
							},
							{
								.name = "diffuse_rect",
								.location = 8,
								.offset = offsetof(struct mesh_instance_data, diffuse_rect),
								.type = pipeline_attribute_vec4
							},
						},
						.count = 6,
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
						{
							.name     = "diffuse_atlas",
							.binding  = 2,
							.stage    = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = renderer->diffuse_atlas->texture
							}
						}
					},
					.count = 3,
				}
			},
			.count = 1
		}
	);

}

struct renderer* new_renderer(const struct framebuffer* framebuffer) {
	struct renderer* renderer = core_calloc(1, sizeof *renderer);
	renderer->framebuffer = framebuffer;

	renderer->diffuse_atlas = new_atlas(texture_flags_filter_linear);

	create_pipeline(renderer);

	return renderer;
}

void free_renderer(struct renderer* renderer) {
	video.free_pipeline(renderer->pipeline);

	free_atlas(renderer->diffuse_atlas);

	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		struct mesh_instance* instance = table_get(renderer->drawlist, *i);

		deinit_vertex_vector(&instance->data);
	}
	free_table(renderer->drawlist);

	core_free(renderer);
}

void renderer_begin(struct renderer* renderer) {
	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		struct mesh_instance* instance = table_get(renderer->drawlist, *i);
		instance->count = 0;

		instance->data.count = 0;
	}
}

void renderer_end(struct renderer* renderer) {

}

void renderer_push(struct renderer* renderer, struct mesh* mesh, struct material* material, m4f transform) {
	struct mesh_instance* instance = table_get(renderer->drawlist, mesh);
	if (!instance) {
		table_set(renderer->drawlist, mesh, (struct mesh_instance) { 0 });
		instance = table_get(renderer->drawlist, mesh);

		init_vertex_vector(&instance->data, sizeof(struct mesh_instance_data), 8);
	}

	v4i* diffuse_atlas_rect = table_get(renderer->diffuse_atlas->rects, material->diffuse_map);
	if (!diffuse_atlas_rect) {
		if (atlas_add_texture(renderer->diffuse_atlas, material->diffuse_map)) {
			video.free_pipeline(renderer->pipeline);
			create_pipeline(renderer);
			return;
		}

		diffuse_atlas_rect = table_get(renderer->diffuse_atlas->rects, material->diffuse_map);
	}

	struct mesh_instance_data data = {
		.r0 = make_v4f(transform.m[0][0], transform.m[0][1], transform.m[0][2], transform.m[0][3]),
		.r1 = make_v4f(transform.m[1][0], transform.m[1][1], transform.m[1][2], transform.m[1][3]),
		.r2 = make_v4f(transform.m[2][0], transform.m[2][1], transform.m[2][2], transform.m[2][3]),
		.r3 = make_v4f(transform.m[3][0], transform.m[3][1], transform.m[3][2], transform.m[3][3]),
		.diffuse = material->diffuse,
		.diffuse_rect = make_v4f(diffuse_atlas_rect->x, diffuse_atlas_rect->y, diffuse_atlas_rect->z, diffuse_atlas_rect->w)
	};

	vertex_vector_push(&instance->data, &data, 1);

	instance->count++;
}

void renderer_finalise(struct renderer* renderer, struct camera* camera) {
	renderer->vertex_config.view = get_camera_view(camera);
	renderer->vertex_config.projection = get_camera_projection(camera);
	renderer->vertex_config.atlas_size = make_v2f(renderer->diffuse_atlas->size.x, renderer->diffuse_atlas->size.y);

	renderer->fragment_config.camera_pos = camera->position;

	video.update_pipeline_uniform(renderer->pipeline, "primary", "VertexConfig",   &renderer->vertex_config);
	video.update_pipeline_uniform(renderer->pipeline, "primary", "FragmentConfig", &renderer->fragment_config);

	video.begin_pipeline(renderer->pipeline);
		for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
			struct mesh_instance* instance = table_get(renderer->drawlist, *i);
			struct mesh* mesh = *i;

			video.bind_vertex_buffer(mesh->vb,              renderer_vert_buffer_bind_point);
			video.bind_vertex_buffer(instance->data.buffer, renderer_inst_buffer_bind_point);
			video.bind_index_buffer(mesh->ib);
			video.bind_pipeline_descriptor_set(renderer->pipeline, "primary", 0);
			video.draw_indexed(mesh->count, 0, instance->count);
		}
	video.end_pipeline(renderer->pipeline);
}
