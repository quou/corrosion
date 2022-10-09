#pragma once

#include "common.h"
#include "core.h"
#include "res.h"
#include "video.h"

/* The atlas is a structure that allows combining textures into a
 * single texture and acessing them by pointer. For this to work,
 * textures must be kept alive for the entire period that they are
 * to be accessed by an atlas, which is wasteful on memory. This
 * therefore shouldn't be used for large textures. */

struct atlas {
	struct texture* texture;
	v2i size;

	u32 flags;

	table(const struct texture*, v4i) rects;
};

struct atlas* new_atlas(u32 flags);
void free_atlas(struct atlas* atlas);

void reset_atlas(struct atlas* atlas);

/* Returns true if the texture was re-created. */
bool atlas_add_texture(struct atlas* atlas, const struct texture* texture);
void atlas_update_texture(struct atlas* atlas, const struct texture* texture);
