#pragma once

#include <corrosion/cr.h>

typedef u32 voxel_t;

struct chunk {
	v3i position;
	v3u extent;

	voxel_t* voxels;
};

void init_chunk(struct chunk* chunk, v3i pos, v3u extent);
void init_chunk_from_slices(struct chunk* chunk, v3i pos, const struct image* image);
void deinit_chunk(struct chunk* chunk);

static inline usize chunk_idx(const struct chunk* chunk, v3u pos) {
	return (chunk->extent.x * chunk->extent.y * pos.z) + (chunk->extent.y * pos.y) + pos.x;
}

static inline void chunk_set(struct chunk* chunk, v3u pos, v3f colour) {
	/* First three bytes are the colour, the last byte is the state */
	u32 fill = (u32)(
		((u8)(colour.r * 255.0f) << 24) |
		((u8)(colour.g * 255.0f) << 16) |
		((u8)(colour.b * 255.0f) << 8)  |
		(u8)1
	);

	chunk->voxels[chunk_idx(chunk, pos)] = fill;
}

static inline void chunk_unset(struct chunk* chunk, v3u pos) {
	chunk->voxels[chunk_idx(chunk, pos)] = 0;
}

static inline usize chunk_size(const struct chunk* chunk) {
	return chunk->extent.x * chunk->extent.y * chunk->extent.z * sizeof *chunk->voxels;
}

void copy_chunk_to_storage(struct storage* storage, const struct chunk* chunk, usize offset);
