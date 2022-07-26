#include <stdlib.h>

#include "core.h"

#ifdef debug
usize memory_usage = 0;

struct alloc_info {
	const char* file;
	u32 line;
	usize size;

	struct alloc_info* next;
	struct alloc_info* prev;
};

struct alloc_info* alloc_head;
struct alloc_info* alloc_tail;

void alloc_init() {
	alloc_head = null;
	alloc_tail = null;
}

void alloc_deinit() {

}

static void alloc_insert(struct alloc_info* node, struct alloc_info* new_node) {
	if (node) {
		new_node->next = node->next;
		new_node->prev = node;

		if (node->next) {
			node->next->prev = new_node;
		}
		node->next = new_node;
	} else {
		if (!alloc_head) {
			alloc_head = new_node;
			alloc_head->next = null;
			alloc_head->prev = null;
		} else {
			alloc_head->prev = new_node;
			new_node->next = alloc_head;
			alloc_head = new_node;
		}
	}

	if (!alloc_tail) {
		alloc_tail = new_node;
		alloc_tail->next = null;
		alloc_tail->prev = null;
	}

	if (node == alloc_tail) {
		alloc_tail = new_node;
	}
}

static void alloc_add(struct alloc_info* info) {
	alloc_insert(alloc_tail, info);
}

static void alloc_remove(struct alloc_info* node) {
	if (!alloc_head || !node) { return; }

	if (alloc_head == node) {
		alloc_head = node->next;
	}

	if (alloc_tail == node) {
		alloc_tail = node->prev;
	}

	if (node->next) {
		node->next->prev = node->prev;
	}

	if (node->prev) {
		node->prev->next = node->next;
	}
}

void* _core_alloc(usize size, struct alloc_code_info cinfo) {
	u8* ptr = malloc(sizeof(struct alloc_info) + size);

	if (!ptr) {
		abort_with("Out of memory.");
	}

	struct alloc_info* info = (void*)ptr;
	info->file = cinfo.file;
	info->line = cinfo.line;
	info->size = size;

	alloc_add(info);

	memory_usage += size;

	void* r = ptr + sizeof *info;

	return r;
}

void* _core_calloc(usize count, usize size, struct alloc_code_info cinfo) {
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

	memory_usage += alloc_size;

	void* r = ptr + sizeof *info;

	return r;
}

void* _core_realloc(void* p, usize size, struct alloc_code_info cinfo) {
	u8* ptr = p;

	if (ptr) {
		struct alloc_info* old_info = (void*)(ptr - sizeof *old_info);
		memory_usage -= old_info->size;
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

	memory_usage += size;

	void* r = new_ptr + sizeof *info;

	return r;
}

void _core_free(void* p, struct alloc_code_info info) {
	if (!p) { return; }
	
	u8* ptr = p;

	struct alloc_info* old_info = (void*)(ptr - sizeof *old_info);
	memory_usage -= old_info->size;

	alloc_remove(old_info);

	free(old_info);
}

usize core_get_memory_usage() {
	return memory_usage;
}

void leak_check() {
	if (core_get_memory_usage() != 0) {
		struct alloc_info* node = alloc_head;
		while (node) {
			warning("Leaked block of %llu bytes allocated at: %s:%u", node->size, node->file, node->size);

			node = node->next;
		}

		info("Total leaked memory: %llu", memory_usage);
	} else {
		info("No leaks from tracked allocations.");
	}
}

#endif
