#include <stdlib.h>

#include "core.h"
#include "thread.h"

usize memory_usage = 0;

/* Memory allocator for use in debug mode that checks for memory leaks.
 *
 * This works by storing and "alloc_info" struct behind the allocation
 * and adding that struct to a linked list each time a block is allocated,
 * with the struct being removed from the list on each deallocation.
 * The leak_check function traverses the list and anything that's still
 * in the list is reported as leaked memory.
 *
 * This allocator is thread-safe. */

struct alloc_info {
	const char* file;
	u32 line;
	usize size;

	struct alloc_info* next;
	struct alloc_info* prev;
};

list(struct alloc_info) alloc_list;
struct mutex list_mutex;

void debug_alloc_init() {
	memset(&alloc_list, 0, sizeof alloc_list);
	init_mutex(&list_mutex);
}

void debug_alloc_deinit() {
	deinit_mutex(&list_mutex);
}

static void alloc_insert(struct alloc_info* node, struct alloc_info* new_node) {
	lock_mutex(&list_mutex);
	list_insert(alloc_list, node, new_node);
	unlock_mutex(&list_mutex);
}

static void alloc_add(struct alloc_info* info) {
	lock_mutex(&list_mutex);

	heap_allocation_count++;
	memory_usage += info->size;
	list_push(alloc_list, info);

	unlock_mutex(&list_mutex);
}

static void alloc_remove(struct alloc_info* node) {
	lock_mutex(&list_mutex);

	memory_usage -= node->size;
	list_remove(alloc_list, node);

	unlock_mutex(&list_mutex);
}

void* debug_core_alloc(usize size, struct alloc_code_info cinfo) {
	u8* ptr = malloc(sizeof(struct alloc_info) + size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	struct alloc_info* info = (void*)ptr;
	info->file = cinfo.file;
	info->line = cinfo.line;
	info->size = size;

	alloc_add(info);

	void* r = ptr + sizeof *info;

	return r;
}

void* debug_core_calloc(usize count, usize size, struct alloc_code_info cinfo) {
	usize alloc_size = count * size;

	u8* ptr = malloc(sizeof(struct alloc_info) + alloc_size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	memset(ptr, 0, sizeof(struct alloc_info) + alloc_size);

	struct alloc_info* info = (void*)ptr;
	info->file = cinfo.file;
	info->line = cinfo.line;
	info->size = alloc_size;

	alloc_add(info);

	void* r = ptr + sizeof *info;

	return r;
}

void* debug_core_realloc(void* p, usize size, struct alloc_code_info cinfo) {
	u8* ptr = p;

	if (ptr) {
		struct alloc_info* old_info = (void*)(ptr - sizeof *old_info);
		alloc_remove(old_info);
	}

	u8* new_ptr = realloc(ptr ? ptr - sizeof(struct alloc_info) : null, sizeof(struct alloc_info) + size);

	if (!new_ptr) {
		abort_with("Out of memory.");
	}

	struct alloc_info* info = (void*)new_ptr;
	info->file = cinfo.file;
	info->line = cinfo.line;
	info->size = size;

	alloc_add(info);

	void* r = new_ptr + sizeof *info;

	return r;
}

void debug_core_free(void* p, struct alloc_code_info info) {
	if (!p) { return; }
	
	u8* ptr = p;

	struct alloc_info* old_info = (void*)(ptr - sizeof *old_info);

	alloc_remove(old_info);

	free(old_info);
}

usize debug_core_get_memory_usage() {
	return memory_usage;
}

void debug_leak_check() {
	if (debug_core_get_memory_usage() != 0) {
		lock_mutex(&list_mutex);

		struct alloc_info* node = alloc_list.head;
		while (node) {
			warning("Leaked block of %llu bytes allocated at: %s:%u", node->size, node->file, node->line);

			node = node->next;
		}

		unlock_mutex(&list_mutex);

		info("Total leaked memory: %llu", memory_usage);
	} else {
		info("No leaks from tracked allocations.");
	}
}

usize release_core_get_memory_usage() { return 0; }

void release_alloc_init() {}
void release_alloc_deinit() {}
void release_leak_check() {}

void* release_core_alloc(usize size) {
	void* ptr = malloc(size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void* release_core_calloc(usize count, usize size) {
	void* ptr = calloc(count, size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void* release_core_realloc(void* p, usize size) {
	void* ptr = realloc(p, size);

	if (!ptr && size != 0) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void release_core_free(void* p) {
	if (p) { free(p); }
}

static usize impl_align_address(usize address, usize alignment) {
	usize mask = alignment - 1;
#if debug
	if ((alignment & mask) != 0) {
		abort_with("Alignment should be a power of two.");
	}
#endif
	return (address + mask) & ~mask;
}

void* align_address(void* address, usize alignment) {
	return (void*)impl_align_address((usize)address, alignment);
}

void* aligned_alloc(usize size, usize alignment) {
	usize actual_size = size + alignment;

	u8* raw = core_alloc(actual_size);

	u8* aligned = align_address(raw, alignment);
	if (aligned == raw) {
		aligned += alignment;
	}

	usize shift = aligned - raw;
	aligned[-1] = (u8)(shift & 0xff);

	return aligned;
}

void aligned_free(void* ptr) {
	if (ptr) {
		u8* aligned = ptr;

		usize shift = aligned[-1];
		if (shift == 0) { shift = 256; }

		u8* raw = aligned - shift;

		core_free(raw);
	}
}
