#include <stdlib.h>
#include <stdio.h>

#include "core.h"

u32 table_lookup_count;
u32 heap_allocation_count;

u64 elf_hash(const u8* data, usize size) {
	usize hash = 0, x = 0;

	for (usize i = 0; i < size; i++) {
		hash = (hash << 4) + data[i];
		if ((x = hash & 0xF000000000LL) != 0) {
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}

	return (hash & 0x7FFFFFFFFF);
}

u64 hash_string(const char* str) {
	return elf_hash((const u8*)str, strlen(str));
}

u64 buffer_id(const u8* data, usize size) {
	u64 id = 0;

	for (usize i = 0; i < size; i++) {
		id += data[i];
	}

	return id;
}

u64 string_id(const char* str) {
	return buffer_id((const u8*)str, strlen(str));
}

static inline u64 hash_key(const u8* bytes, usize size, table_hash_fun f) {
	return f ? f(bytes, size) : elf_hash(bytes, size);
}

static inline bool compare_keys(const void* a, const void* b, usize size, table_compare_fun f) {
	return f ? f((const u8*)a, (const u8*)b) : (memcmp(a, b, size) == 0);
}

static inline bool is_el_null(const u8* el, usize off) {
	return !*(bool*)(el + off);
}

void* _find_table_el(table_hash_fun hash, table_compare_fun compare, void* els_v, usize el_size, usize capacity,
	usize key_size, const void* key_ptr, usize key_off, usize val_off, usize state_off, usize isnt_null_off, usize* ind) {
	usize idx = hash_key(key_ptr, key_size, hash) % capacity;

	u8* tombstone = null;

	u8* els = els_v;

	table_lookup_count++;

	/* Find the first element without a key. Otherwise, finds the first element
	 * with a key matching key_ptr, of course taking into account the table's
	 * compare function pointer. */
	for (;;) {
		u8* el = els + idx * el_size;
		if (is_el_null(el, isnt_null_off)) {
			if (*(el + state_off) != table_el_state_inactive) {
				if (ind) { *ind = idx; }
				return tombstone != null ? tombstone : el;
			} else if (tombstone == null) {
				tombstone = el;
			}
		} else if (compare_keys(el + key_off, key_ptr, key_size, compare)) {
			if (ind) { *ind = idx; }
			return el;
		}

		idx = (idx + 1) % capacity;
	}
}

void* _table_get(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity, usize count,
	usize key_size, const void* key, usize key_off, usize val_off, usize state_off, usize isnt_null_off) {
	if (count == 0) { return null; }

	u8* el = _find_table_el(hash, compare, els, el_size, capacity, key_size, key, key_off, val_off, state_off, isnt_null_off, null);
	if (is_el_null(el, isnt_null_off)) {
		return null;
	}

	return el + val_off;
}

void* _table_first_key(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize count, usize key_size, usize key_off, usize val_off, usize state_off, usize isnt_null_off) {
	if (count == 0) { return null; }

	for (usize i = 0; i < capacity; i++) {
		u8* el = (u8*)els + i * el_size;
		if (!is_el_null(el, isnt_null_off)) {
			return el + key_off;
		}
	}

	return null;
}

void* _table_next_key(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize count, usize key_size, const void* key, usize key_off, usize val_off, usize state_off, usize isnt_null_off) {
	if (count == 0) { return null; }

	usize idx = 0;

	u8* el = _find_table_el(hash, compare, els, el_size, capacity, key_size, key, key_off, val_off, state_off, isnt_null_off, &idx);
	if (is_el_null(el, isnt_null_off)) {
		return null;
	}

	if (idx >= capacity) {
		return null;
	}

	for (usize i = idx + 1; i < capacity; i++) {
		el = (u8*)els + i * el_size;
		if (!is_el_null(el, isnt_null_off)) {
			return el + key_off;
		}
	}

	return null;
}

char* copy_string(const char* str) {
	usize len = strlen(str);
	char* r = core_alloc(len + 1);

	memcpy(r, str, len);
	r[len] = '\0';

	return r;
}
