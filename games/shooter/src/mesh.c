#include "renderer.h"
#include "ufbx.h"

static struct mesh process_mesh(ufbx_scene* scene, ufbx_mesh* mesh) {

}

void init_model_from_fbx(struct model* model, const u8* raw, usize raw_size) {
	ufbx_load_opts opts = {
		.load_external_files = true,
		.allow_null_material = true,
		.generate_missing_normals = true,

		.target_axes = {
			.right = UFBX_COORDINATE_AXIS_POSITIVE_X,
			.up = UFBX_COORDINATE_AXIS_POSITIVE_Y,
			.front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
		},
		.target_unit_meters = 1.0f,
	};

	ufbx_scene* scene = ufbx_load_memory(raw, raw_size, null, null);

	vector_allocate(model->meshes, scene->meshes.count);
	for (usize i = 0; i < scene->meshes.count; i++) {
		vector_push(model->meshes, process_mesh(scene, scene->meshes.data[i]));
	}

	ufbx_free_scene(scene);
}

void deinit_model(struct model* model) {
	for (usize i = 0; i < vector_count(model->meshes); i++) {
		video.free_vertex_buffer(model->meshes[i].vb);
		video.free_index_buffer(model->meshes[i].ib);
	}

	free_vector(model->meshes);
}
