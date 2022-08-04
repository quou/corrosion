#include "entity.h"
#include "renderer.h"

struct world* new_world(struct renderer* renderer) {
	struct world* world = core_calloc(1, sizeof *world);

	world->avail_entity_count = 0;

	world->time = 0.0;

	for (usize i = 0; i < max_entities; i++) {
		world->avail_entities[i] = i;
	}

	world->avail_entity_count = max_entities;

	world->renderer = renderer;

	world->camera = (struct camera) {
		.fov = 70.0f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
		.position.z = -5.0f
	};

	return world;
}

void free_world(struct world* world) {
	core_free(world);
}

void update_world(struct world* world, f64 ts) {
	const v2i fb_size = video.get_framebuffer_size(world->renderer->scene_fb);
	const f32 aspect = (f32)fb_size.x / (f32)fb_size.y;

	m4f vp = m4f_mul(get_camera_projection(&world->camera, aspect), get_camera_view(&world->camera));

	struct frustum_plane fplanes[6];
	compute_frustum_planes(vp, fplanes);

	world->culled = 0;

	for (usize i = 0; i < max_entities; i++) {
		struct entity* e = &world->entities[i];

		if (!e->active) { continue; }

		if (e->behaviour & eb_spin) {
			e->transform = m4f_mul(e->transform, m4f_rotation(euler(make_v3f(0.0f, (f32)ts * e->spin_speed, 0.0f))));
		}

		if (e->behaviour & eb_mesh) {
			for (usize j = 0; j < vector_count(e->model->meshes); j++) {
				struct mesh* mesh = &e->model->meshes[j];

				for (usize x = 0; x < vector_count(mesh->instances); x++) {
					m4f t = m4f_mul(e->transform, e->model->nodes[mesh->instances[x]].transform);

					struct aabb mesh_bound = transform_aabb(&mesh->bound, t);
					if (in_frustum(&mesh_bound, fplanes)) {
						renderer_push(world->renderer, mesh, &e->material, t);
					} else {
						world->culled++;
					}
				}
			}
		}

		if (e->behaviour & eb_light) {
			v4f lp4 = make_v4f(e->light.position.x, e->light.position.y, e->light.position.z, 1.0);
			lp4 = m4f_transform(e->transform, lp4);
			struct light l = e->light;
			l.position = make_v3f(lp4.x, lp4.y, lp4.z);
			renderer_push_light(world->renderer, &l);
		}
	}

	world->time += ts;
}

static void push_avail(struct world* world, usize idx) {
	world->avail_entities[world->avail_entity_count++] = idx;
}

static usize pop_avail(struct world* world) {
	if (world->avail_entity_count == 0) {
		abort_with("Too many entities.");
	}

	return world->avail_entities[--world->avail_entity_count];
}

struct entity* new_entity(struct world* world, enum entity_behaviour behaviour) {
	usize idx = pop_avail(world);

	struct entity* e = &world->entities[idx];
	memset(e, 0, sizeof *e);

	e->transform = m4f_identity();
	e->active = true;
	e->behaviour = behaviour;

	return e;
}

void destroy_entity(struct world* world, struct entity* entity) {
	usize idx = entity - world->entities;

	entity->active = false;

	push_avail(world, idx);
}
