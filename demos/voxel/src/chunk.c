#include "chunk.h"

void init_chunk(struct chunk* chunk, v3i pos, v3u extent) {
	chunk->position = pos;
	chunk->extent = extent;

	chunk->voxels = core_calloc(1, chunk_size(chunk));
}

void init_chunk_from_slices(struct chunk* chunk, v3i pos, const struct image* image) {
	chunk->position = pos;
	chunk->extent = make_v3u(32, 32, 32);

	chunk->voxels = core_calloc(1, chunk_size(chunk));

	u32 x_offset = 0;
	for (u32 y = 0; y < chunk->extent.y; y++) {
		for (u32 x = 0; x < chunk->extent.x; x++) {
			for (u32 z = 0; z < chunk->extent.z; z++) {
				u8* pixel = (u8*)&image->colours[(x + x_offset + z * image->size.x) * 4];
				u8 r = *(pixel + 0);
				u8 g = *(pixel + 1);
				u8 b = *(pixel + 2);
				u8 a = *(pixel + 3);

				if (a != 0) {
					chunk_set(chunk, make_v3u(x, (31 - y), z), make_v3f(
						(f32)r / 255.0f,
						(f32)g / 255.0f,
						(f32)b / 255.0f
					));
				}
			}
		}

		x_offset += chunk->extent.x;
	}
}

void deinit_chunk(struct chunk* chunk) {
	core_free(chunk->voxels);
}

void copy_chunk_to_storage(struct storage* storage, const struct chunk* chunk, usize offset) {
	video.update_storage_region(storage, chunk->voxels, offset, chunk_size(chunk));
}
