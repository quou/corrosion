#pragma once

#include "common.h"
#include "core.h"
#include "res.h"
#include "video.h"

struct atlas {
	struct texture* texture;
	v2i size;

	u32 flags;

	table(const struct texture*, v4i) rects;
};

struct atlas* new_atlas(u32 flags);
void free_atlas(struct atlas* atlas);

/* Returns true if the texture was re-created. */
bool atlas_add_texture(struct atlas* atlas, const struct texture* texture);
