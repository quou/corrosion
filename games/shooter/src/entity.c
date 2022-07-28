#include "entity.h"
#include "renderer.h"

struct world* new_world(struct renderer* renderer) {
	struct world* world = core_alloc(sizeof *world);

	world->avail_entity_count = 0;

	for (usize i = 0; i < max_entities; i++) {
		world->avail_entities[i] = i;
	}

	world->avail_entity_count = max_entities;

	world->renderer = renderer;

	return world;
}

void free_world(struct world* world) {
	core_free(world);
}

void update_world(struct world* world, f64 ts) {
	for (usize i = 0; i < max_entities; i++) {
		struct entity* e = world->entities + i;

		if (!e->active) { continue; }
	}
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

	e->behaviour = behaviour;

	return e;
}

void destroy_entity(struct world* world, struct entity* entity) {
	usize idx = entity - world->entities;

	entity->active = false;

	push_avail(world, idx);
}
