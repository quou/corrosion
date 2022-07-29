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
	for (usize i = 0; i < max_entities; i++) {
		struct entity* e = world->entities + i;

		if (!e->active) { continue; }

		if (e->behaviour & eb_mesh) {
			for (usize j = 0; j < vector_count(e->model->meshes); j++) {
				struct mesh* mesh = &e->model->meshes[j];

				renderer_push(world->renderer, mesh, e->transform);
			}
		}

		if (e->behaviour & eb_spin) {
			e->transform = m4f_mul(e->transform, m4f_rotation(euler(make_v3f(0.0f, (f32)ts * 25.0f, 0.0f))));
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

	return world->avail_entities[world->avail_entity_count--];
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