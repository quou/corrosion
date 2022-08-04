#include "renderer.h"
#include "ufbx.h"

static struct model_node process_node(ufbx_scene* scene, ufbx_node* node) {
	struct model_node rnode = { 0 };

	ufbx_matrix m = node->geometry_to_world;
	ufbx_vec3 t = node->world_transform.translation;

	rnode.transform = (m4f) {
		(f32)m.m00, (f32)m.m01, (f32)m.m02, (f32)m.m03,
		(f32)m.m10, (f32)m.m11, (f32)m.m12, (f32)m.m13,
		(f32)m.m20, (f32)m.m21, (f32)m.m22, (f32)m.m23,
		(f32)t.x,   (f32)t.y,   (f32)t.z,   1.0f,
	};

	return rnode;
}

static struct mesh process_mesh(ufbx_scene* scene, ufbx_mesh* mesh) {
	struct mesh rmesh = { 0 };

	rmesh.bound.min = make_v3f( INFINITY,  INFINITY,  INFINITY);
	rmesh.bound.max = make_v3f(-INFINITY, -INFINITY, -INFINITY);

	vector(u32) tri_indices = null;
	vector_allocate(tri_indices, mesh->max_face_triangles * 3);

	vector(struct mesh_vertex) vertices = null;
	vector(u32) indices = null;

	for (usize i = 0; i < mesh->faces.count; i++) {
		ufbx_face face = mesh->faces.data[i];
		usize tri_count = ufbx_triangulate_face(tri_indices, mesh->max_face_triangles * 3, mesh, face);

		ufbx_vec2 default_uv = { 0 };

		for (usize j = 0; j < tri_count * 3; j++) {
			u32 idx = tri_indices[j];

			ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, idx);
			ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, idx);
			ufbx_vec2 uv = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, idx) : default_uv;

			rmesh.bound.min.x = cr_min((f32)pos.x, rmesh.bound.min.x);
			rmesh.bound.min.y = cr_min((f32)pos.y, rmesh.bound.min.y);
			rmesh.bound.min.z = cr_min((f32)pos.z, rmesh.bound.min.z);
			rmesh.bound.max.x = cr_max((f32)pos.x, rmesh.bound.max.x);
			rmesh.bound.max.y = cr_max((f32)pos.y, rmesh.bound.max.y);
			rmesh.bound.max.z = cr_max((f32)pos.z, rmesh.bound.max.z);

			struct mesh_vertex v = {
				.position = make_v3f(pos.x,    pos.y,    pos.z),
				.normal   = make_v3f(normal.x, normal.y, normal.z),
				.uv       = make_v2f(uv.x, uv.y)
			};

			vector_push(vertices, v);

			vector_push(indices, 0);
		}
	}

	ufbx_vertex_stream streams[] = {
		{
			streams[0].data = vertices,
			streams[0].vertex_size = sizeof(struct mesh_vertex),
		}
	};

	ufbx_error r;
	usize vertex_count = ufbx_generate_indices(streams, 1, indices, vector_count(indices), null, &r);
	if (r.type != UFBX_ERROR_NONE) {
		abort_with("Failed to generate mesh indices.");
	}

	vector_allocate(rmesh.instances, mesh->instances.count);
	for (usize i = 0; i < mesh->instances.count; i++) {
		vector_push(rmesh.instances, (usize)mesh->instances.data[i]->typed_id);
	}

	rmesh.vb = video.new_vertex_buffer(vertices, vertex_count * sizeof *vertices, vertex_buffer_flags_none);
	rmesh.ib = video.new_index_buffer(indices, vector_count(indices), index_buffer_flags_u32);

	rmesh.count = vector_count(indices);

	free_vector(vertices);
	free_vector(indices);
	free_vector(tri_indices);

	return rmesh;
}

void init_model_from_fbx(struct model* model, const u8* raw, usize raw_size) {
	memset(model, 0, sizeof *model);

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
		.strict = true
	};

	ufbx_scene* scene = ufbx_load_memory(raw, raw_size, null, null);

	vector_allocate(model->nodes, scene->nodes.count);
	for (usize i = 0; i < scene->nodes.count; i++) {
		vector_push(model->nodes, process_node(scene, scene->nodes.data[i]));
	}

	vector_allocate(model->meshes, scene->meshes.count);
	for (usize i = 0; i < scene->meshes.count; i++) {
		vector_push(model->meshes, process_mesh(scene, scene->meshes.data[i]));
	}

	ufbx_free_scene(scene);
}

void deinit_model(struct model* model) {
	for (usize i = 0; i < vector_count(model->meshes); i++) {
		if (model->meshes[i].vb) { video.free_vertex_buffer(model->meshes[i].vb); }
		if (model->meshes[i].ib) { video.free_index_buffer(model->meshes[i].ib); }

		free_vector(model->meshes[i].instances);
	}

	free_vector(model->meshes);
	free_vector(model->nodes);
}
