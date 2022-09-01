#include "chunk.h"

void init_chunk(struct chunk* chunk, v3i pos, v3u extent) {
	chunk->position = pos;
	chunk->extent = extent;

	chunk->voxels = core_calloc(1, chunk_size(chunk));
}

void deinit_chunk(struct chunk* chunk) {
	core_free(chunk->voxels);
}

void copy_chunk_to_storage(struct storage* storage, const struct chunk* chunk, usize offset) {
	video.update_storage_region(storage, chunk->voxels, offset, chunk_size(chunk));
}
