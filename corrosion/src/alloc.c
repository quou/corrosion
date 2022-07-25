#include <stdlib.h>

#define table_malloc malloc
#define table_free free

#include "core.h"

#ifdef debug
usize memory_usage = 0;

struct alloc_info {
	const char* file;
	u32 line;
	usize size;
};

table(void*, struct alloc_info) allocations;

void alloc_init() {
	memset(&allocations, 0, sizeof allocations);
}

void alloc_deinit() {
	free_table(allocations);
}

void* _core_alloc(usize size, struct alloc_code_info info) {
	u8* ptr = malloc(sizeof(usize) + size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	memcpy(ptr, &size, sizeof(usize));

	memory_usage += size;

	void* r = ptr + sizeof(usize);

	table_set(allocations, r, ((struct alloc_info) { info.file, info.line, size }));

	return r;
}

void* _core_calloc(usize count, usize size, struct alloc_code_info info) {
	usize alloc_size = count * size;

	u8* ptr = malloc(sizeof(usize) + alloc_size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	memset(ptr + sizeof(usize), 0, alloc_size);

	memcpy(ptr, &alloc_size, sizeof(usize));

	memory_usage += alloc_size;

	void* r = ptr + sizeof(usize);

	table_set(allocations, r, ((struct alloc_info) { info.file, info.line, size }));

	return r;
}

void* _core_realloc(void* p, usize size, struct alloc_code_info info) {
	u8* ptr = p;

	if (ptr) {
		usize* old_size = (usize*)(ptr - sizeof(usize));
		memory_usage -= *old_size;

		table_delete(allocations, p);
	}

	u8* new_ptr = realloc(ptr ? ptr - sizeof(usize) : null, sizeof(usize) + size);

	if (!new_ptr) {
		abort_with("Out of memory.");
	}

	memcpy(new_ptr, &size, sizeof(usize));

	memory_usage += size;

	void* r = new_ptr + sizeof(usize);

	table_set(allocations, r, ((struct alloc_info) { info.file, info.line, size }));

	return r;
}

void _core_free(void* p, struct alloc_code_info info) {
	if (!p) { return; }
	
	u8* ptr = p;

	usize* old_size = (usize*)(ptr - sizeof(usize));
	memory_usage -= *old_size;

	table_delete(allocations, p);

	free(old_size);
}

usize core_get_memory_usage() {
	return memory_usage;
}

void leak_check() {
	if (core_get_memory_usage() != 0) {
		for (void** i = table_first(allocations); i; i = table_next(allocations, *i)) {
			struct alloc_info* info = table_get(allocations, *i);

			warning("Memory leak of size %llu, allocated at: %s:%u", info->size, info->file, info->line);
		}
		info("Total leaked memory: %llu", memory_usage);
	}
}

#endif
