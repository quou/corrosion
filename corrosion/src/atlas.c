#include "atlas.h"

struct atlas* new_atlas(u32 flags) {
	struct atlas* atlas = core_calloc(1, sizeof(struct atlas));

	struct image image = {
		.colours = null,
		.size = { 256, 256 }
	};

	atlas->flags = flags;

	atlas->texture = video.new_texture(&image, flags);
	v2i size = image.size;

	return atlas;
}

void free_atlas(struct atlas* atlas) {
	video.free_texture(atlas->texture);

	free_table(atlas->rects);

	core_free(atlas);
}

bool atlas_add_texture(struct atlas* atlas, const struct texture* new_texture) {
	/* TODO: Proper packing algorithm. */

	bool recreated = false;

	table_set(atlas->rects, new_texture, make_v4i(0, 0, 0, 0));

	v2i final_size = make_v2i(0, 0);
	for (const struct texture** texture_ptr = table_first(atlas->rects);
		texture_ptr;
		texture_ptr = table_next(atlas->rects, *texture_ptr)) {
		const struct texture* texture = *texture_ptr;

		v2i size = video.get_texture_size(texture);

		final_size.x += size.x;
		final_size.y = max(final_size.y, size.y);
	}

	if (final_size.x > atlas->size.x || final_size.y > atlas->size.y) {
		/* The atlas needs more space, re-create the destination texture. */

		video.free_texture(atlas->texture);
		struct image image = {
			.colours = null,
			.size = final_size
		};

		atlas->texture = video.new_texture(&image, atlas->flags);
		atlas->size = final_size;

		recreated = true;
	}

	v2i dst_pos = make_v2i(0, 0);

	for (struct texture** texture_ptr = table_first(atlas->rects);
		texture_ptr;
		texture_ptr = table_next(atlas->rects, *texture_ptr)) {
		struct texture* texture = *texture_ptr;
		v4i* dst_rect = table_get(atlas->rects, texture);

		v2i size = video.get_texture_size(texture);

		dst_rect->x = dst_pos.x;
		dst_rect->y = dst_pos.y;
		dst_rect->z = size.x;
		dst_rect->w = size.y;

		video.texture_copy(atlas->texture, dst_pos, texture, make_v2i(0, 0), size);

		dst_pos.x += size.x;
	}

	return recreated;
}

void atlas_update_texture(struct atlas* atlas, const struct texture* texture) {
	v4i* rect_ptr = table_get(atlas->rects, texture);
	if (!rect_ptr) {
		error("Cannot update an atlas with a texture that has not been added.");
		return;
	}

	v4i rect = *rect_ptr;

	video.texture_copy(atlas->texture, make_v2i(rect.x, rect.y), texture, make_v2i(0, 0), video.get_texture_size(texture));
}
