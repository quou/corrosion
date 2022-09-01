#pragma once

#include <corrosion/cr.h>

typedef u32 voxel_t;

struct chunk {
	v3i position;
	v3u extent;

	voxel_t* voxels;
};

void init_chunk(struct chunk* chunk, v3i pos, v3u extent);
void deinit_chunk(struct chunk* chunk);

static inline usize chunk_idx(const struct chunk* chunk, v3u pos) {
	return (chunk->extent.x * chunk->extent.y * pos.z) + (chunk->extent.y * pos.y) + pos.x;
}

static inline void chunk_set(struct chunk* chunk, v3u pos, v4f colour) {
	chunk->voxels[chunk_idx(chunk, pos)] = 1;
}

static inline usize chunk_size(const struct chunk* chunk) {
	return chunk->extent.x * chunk->extent.y * chunk->extent.z * sizeof *chunk->voxels;
}

void copy_chunk_to_storage(struct storage* storage, const struct chunk* chunk, usize offset);
