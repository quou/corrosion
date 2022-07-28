#pragma once

#include <corrosion/cr.h>

struct renderer;

enum entity_behaviour {
	eb_mesh         = 1 << 0
};

struct entity {
	bool active;

	m4f transform;

	enum entity_behaviour behaviour;
};

#define max_entities 500

struct world {
	struct entity entities[max_entities];
	usize avail_entities[max_entities];
	usize avail_entity_count;
	
	struct renderer* renderer;
};

struct world* new_world(struct renderer* renderer);
void free_world(struct world* world);

void update_world(struct world* world, f64 ts);

struct entity* new_entity(struct world* world, enum entity_behaviour behaviour);
void destroy_entity(struct world* world, struct entity* entity);
