#pragma once

#include <corrosion/cr.h>

#include "renderer.h"

enum entity_behaviour {
	eb_none         = 1 << 0,
	eb_mesh         = 1 << 1,
	eb_spin         = 1 << 2,
	eb_light        = 1 << 3
};

struct entity {
	bool active;

	m4f transform;

	struct model* model;
	struct material material;

	struct light light;

	f32 spin_speed;

	enum entity_behaviour behaviour;
};

#define max_entities 45000

struct world {
	struct entity entities[max_entities];
	usize avail_entities[max_entities];
	usize avail_entity_count;

	f64 time;

	struct renderer* renderer;

	struct camera camera;
};

struct world* new_world(struct renderer* renderer);
void free_world(struct world* world);

void update_world(struct world* world, f64 ts);

struct entity* new_entity(struct world* world, enum entity_behaviour behaviour);
void destroy_entity(struct world* world, struct entity* entity);
