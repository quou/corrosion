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
	if (size == 0) { return null; }

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
	if (count * size == 0) { return null; }

	void* r = debug_core_alloc(count * size, cinfo);

	memset(r, 0, count * size);

	return r;
}

void* debug_core_realloc(void* p, usize size, struct alloc_code_info cinfo) {
	if (size == 0) {
		debug_core_free(p, cinfo);
		return null;
	}

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
	if (size == 0) { return null; }

	void* ptr = malloc(size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void* release_core_calloc(usize count, usize size) {
	if (count * size == 0) { return null; }

	void* ptr = calloc(count, size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void* release_core_realloc(void* p, usize size) {
	if (size == 0) {
		free(p);
		return null;
	}

	void* ptr = realloc(p, size);

	if (!ptr && size != 0) {
		abort_with("Out of memory.");
	}

	return ptr;
}

void release_core_free(void* p) {
	if (p) { free(p); }
}

void* aligned_core_alloc(usize size, usize alignment) {
	usize offset = alignment - 1 + sizeof(void*);

	void* p1 = core_alloc(size + offset);

	void** p2 = (void**)(((uptr)(p1) + offset) & ~(alignment - 1));
	p2[-1] = p1;

	return p2;
}

void aligned_core_free(void* ptr) {
	if (ptr) {
		core_free(((void**)ptr)[-1]);
	}
}
