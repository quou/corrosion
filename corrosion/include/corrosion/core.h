#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include "common.h"

extern u32 table_lookup_count;
extern u32 heap_allocation_count;

u64 elf_hash(const u8* data, usize size);
u64 hash_string(const char* str);

void info(const char* fmt, ...);
void error(const char* fmt, ...);
void warning(const char* fmt, ...);

void vinfo(const char* fmt, va_list args);
void verror(const char* fmt, va_list args);
void vwarning(const char* fmt, va_list args);

void abort_with(const char* fmt, ...);
void vabort_with(const char* fmt, va_list args);

#define cat2(x, y) x ## y
#define cat(x, y) cat2(x, y)

/* Inserts a byte array of size s_ for
 * structures */
#define pad(s_) \
	u8 cat(padding_, __LINE__)[s_]


/* In debug mode, allocations are tracked and memory leaks are reported.
 * This slows down the allocations, however, and uses extra memory, so it's
 * disabled in other configurations. */
struct alloc_code_info {
	const char* file;
	u32 line;
};

void* debug_core_alloc(usize size, struct alloc_code_info info);
void* debug_core_calloc(usize count, usize size, struct alloc_code_info info);
void* debug_core_realloc(void* ptr, usize size, struct alloc_code_info info);
void debug_core_free(void* ptr, struct alloc_code_info info);

void* release_core_alloc(usize size);
void* release_core_calloc(usize count, usize size);
void* release_core_realloc(void* ptr, usize size);
void release_core_free(void* ptr);

usize release_core_get_memory_usage();
void release_alloc_init();
void release_alloc_deinit();
void release_leak_check();

usize debug_core_get_memory_usage();
void debug_alloc_init();
void debug_alloc_deinit();
void debug_leak_check();

#ifdef debug
#define core_alloc(s_) debug_core_alloc(s_, (struct alloc_code_info) { __FILE__, __LINE__ })
#define core_calloc(c_, s_) debug_core_calloc(c_, s_, (struct alloc_code_info) { __FILE__, __LINE__ })
#define core_realloc(p_, s_) debug_core_realloc(p_, s_, (struct alloc_code_info) { __FILE__, __LINE__ })
#define core_free(p_) debug_core_free(p_, (struct alloc_code_info) { __FILE__, __LINE__ })

#define core_get_memory_usage() debug_core_get_memory_usage()
#define alloc_init() debug_alloc_init()
#define alloc_deinit() debug_alloc_deinit()
#define leak_check() debug_leak_check()
#else
#define core_alloc(s_) release_core_alloc(s_)
#define core_calloc(c_, s_) release_core_calloc(c_, s_)
#define core_realloc(p_, s_) release_core_realloc(p_, s_)
#define core_free(p_) release_core_free(p_)

#define core_get_memory_usage() release_core_get_memory_usage()
#define alloc_init() release_alloc_init()
#define alloc_deinit() release_alloc_deinit()
#define leak_check() release_leak_check()
#endif

void* align_address(void* address, usize alignment);
void* aligned_core_alloc(usize size, usize alignment);
void aligned_core_free(void* ptr);

/* String manipulation. */
char* copy_string(const char* str);

/*struct type_info {
	u64 id;
	usize size;
	const char* name;
};

#define type_info(t_) \
	((struct type_info) { \
		.id = elf_hash((const u8*)#t_, sizeof(t_)), \
		.size = sizeof(t_), \
		.name = #t_ \
	})*/

/* Generic hash table.
 *
 * Keys and values can be any type. Tables should be
 * initialised to zero before use.
 *
 * Usage example:
 *
 * table(u32, i32) test_table = { 0 };
 *
 * table_set(test_table, 10, 33);
 * table_set(test_table, 45, 3);
 * table_set(test_table, 16, 5);
 * table_set(test_table, 1, 9);
 *
 * table_delete(test_table, 1);
 *
 * table_set(test_table, 1, 10);
 *
 * for (u32* i = table_first(test_table); i; i = table_next(test_table, *i)) {
 *     printf("%u: %d\n", *i, *(i32*)table_get(test_table, *i));
 * }
 *
 * free_table(test_table);
 */

#define table_load_factor 0.75

/* Takes a struct value instead of a type and gets
 * the offset of a property. */
#define voffsetof(v_, m_) \
	(usize)(((u8*)(&((v_).m_))) - ((u8*)(&(v_))))

enum {
	table_el_state_active = 0, /* A normal element. */
	table_el_state_inactive    /* An element that has been deleted. */
};

typedef u64  (*table_hash_fun)(const u8* bytes, usize size);
typedef bool (*table_compare_fun)(const u8* a, const u8* b);
typedef void (*table_free_key_fun)(u8* ptr);
typedef u8*  (*table_copy_key_fun)(const u8* src);

inline static u64 table_hash_string(const u8* data, usize size) {
	return hash_string(*(const char**)data);
}

inline static bool table_compare_string(const u8* a, const u8* b) {
	return strcmp(*(const char**)a, *(const char**)b) == 0;
}

void table_free_string(u8* ptr);

inline static u8* table_copy_string(const u8* src) {
	return (u8*)copy_string((const char*)src);
}

#define _table_el(kt_, vt_) \
	struct { \
		kt_ key; \
		vt_ value; \
		u8 state; \
		bool isnt_null; \
	}

#define table(kt_, vt_) \
	struct { \
		_table_el(kt_, vt_)* entries; \
		usize count, capacity; \
		\
		/* Temporary element, key and value for assigning to
		 * take a pointer to to allow literals to be passed
		 * into the macros. */\
		_table_el(kt_, vt_) e; \
		table_hash_fun hash; /* If not null, should return the hash of a key. */ \
		table_compare_fun compare; /* If not null, should return true if the keys are equal. */ \
		table_copy_key_fun copy_key;  \
		table_free_key_fun free_key; \
		kt_ k; \
		vt_ v; \
	}

/* Internal table functions, to be called upon by the table macros. */
void* _find_table_el(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize key_size, const void* key, usize key_off, usize val_off, usize state_off, usize isnt_null_off, usize* ind);
void* _table_get(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize count, usize key_size, const void* key, usize key_off, usize val_off, usize state_off, usize isnt_null_off);
void* _table_first_key(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize count, usize key_size, usize key_off, usize val_off, usize state_off, usize isnt_null_off);
void* _table_next_key(table_hash_fun hash, table_compare_fun compare, void* els, usize el_size, usize capacity,
	usize count, usize key_size, const void* key, usize key_off, usize val_off, usize state_off, usize isnt_null_off);

#define _table_is_el_null(t_, el_) \
	(!*(bool*)((el_) + voffsetof((t_).e, isnt_null)))

#define _table_resize(t_, cap_) \
	do { \
		u8* els_ = core_alloc((cap_) * sizeof (t_).e); \
		for (usize i_ = 0; i_ < (cap_); i_++) { \
			els_[(i_ * sizeof (t_).e) + voffsetof((t_).e, isnt_null)] = false; \
			els_[(i_ * sizeof (t_).e) + voffsetof((t_).e, state)]     = table_el_state_active; \
		} \
		\
		(t_).count = 0; \
		for (usize i_ = 0; i_ < (t_).capacity; i_++) { \
			u8* el_ = (((u8*)(t_).entries) + i_ * sizeof (t_).e); \
			if (_table_is_el_null(t_, el_)) { continue; } \
			u8* dst_ = _find_table_el((t_).hash, (t_).compare, els_, sizeof *(t_).entries, (cap_), sizeof (t_).k, \
				el_ + voffsetof(*(t_).entries, key),\
				voffsetof(*(t_).entries, key), voffsetof(*(t_).entries, value), voffsetof((t_).e, state), \
				voffsetof((t_).e, isnt_null), null); \
			memcpy(dst_ + voffsetof((t_).e, key),       el_ + voffsetof((t_).e, key),       sizeof (t_).k); \
			memcpy(dst_ + voffsetof((t_).e, value),     el_ + voffsetof((t_).e, value),     sizeof (t_).v); \
			memcpy(dst_ + voffsetof((t_).e, state),     el_ + voffsetof((t_).e, state),     sizeof(u8)); \
			memcpy(dst_ + voffsetof((t_).e, isnt_null), el_ + voffsetof((t_).e, isnt_null), sizeof(bool)); \
			(t_).count++; \
		} \
		if ((t_).entries) { core_free((t_).entries); } \
		(t_).entries = (void*)els_; \
		(t_).capacity = (cap_); \
	} while (0)

#define free_table(t_) \
	do { \
		if ((t_).entries) { \
			if ((t_).free_key) { \
				for (usize i_ = 0; i_ < (t_).capacity; i_++) { \
					u8* el_ = (((u8*)(t_).entries) + i_ * sizeof (t_).e); \
					if (_table_is_el_null(t_, el_)) { continue; } \
					(t_).free_key(el_ + voffsetof((t_).e, key)); \
				} \
			} \
			core_free((t_).entries); \
		} \
	} while (0)

#define table_set(t_, k_, v_) \
	do { \
		if ((t_).count >= (t_).capacity * table_load_factor) { \
			usize new_cap_ = (t_).capacity < 8 ? 8 : (t_).capacity * 2; \
			_table_resize((t_), new_cap_); \
		} \
		(t_).k = k_; \
		u8* el_ = _find_table_el((t_).hash, (t_).compare, (t_).entries, sizeof *(t_).entries, (t_).capacity, sizeof (t_).k, \
			&(t_).k,\
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), voffsetof((t_).e, isnt_null), null); \
		if (_table_is_el_null(t_, el_)) { \
			if (el_[voffsetof((t_).e, state)] == table_el_state_active) { \
				(t_).count++; \
			} \
			if ((t_).copy_key) { \
				(t_).k = k_; \
				u8* tk_; \
				memcpy(&tk_, &(t_).k, sizeof tk_); \
				tk_ = (t_).copy_key(tk_); \
				memcpy(&(t_).k, &tk_, sizeof tk_); \
			} \
			memcpy(el_ + voffsetof((t_).e, key),   &(t_).k, sizeof (t_).k); \
		} \
		(t_).v = (v_); \
		el_[voffsetof((t_).e, isnt_null)] = true; \
		memcpy(el_ + voffsetof((t_).e, value), &(t_).v, sizeof (t_).v); \
	} while (0)

#define table_get(t_, k_) \
	((t_).k = (k_), \
		_table_get((t_).hash, (t_).compare, (t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			&(t_).k,\
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), voffsetof((t_).e, isnt_null)))

#define table_delete(t_, k_) \
	do { \
		if ((t_).count > 0) { \
			(t_).k = (k_); \
			u8* el_ = _find_table_el((t_).hash, (t_).compare, (t_).entries, sizeof *(t_).entries, (t_).capacity, sizeof (t_).k, \
				&(t_).k,\
				voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), voffsetof((t_).e, isnt_null), null); \
			if (!_table_is_el_null(t_, el_)) { \
				if ((t_).free_key) { \
					(t_).free_key(el_ + voffsetof((t_).e, key)); \
				} \
				el_[voffsetof((t_).e, isnt_null)] = false; \
				el_[voffsetof((t_).e, state)] = table_el_state_inactive; \
			} \
		} \
	} while (0)

#define table_first(t_) \
	_table_first_key((t_).hash, (t_).compare, (t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), voffsetof((t_).e, isnt_null))

#define table_next(t_, k_) \
	((t_).k = (k_), \
		_table_next_key((t_).hash, (t_).compare, (t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			&(t_).k, \
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), voffsetof((t_).e, isnt_null)))

/* Dynamic array.
 *
 * The user maintains a pointer to the first element of the vector; The
 * implementation stores a `vector_header' struct behind that pointer. This
 * way, the user can index into the vector using the default [] operator.
 * Vectors must be initialised to a null pointer before they can be operated on.
 *
 * Example:
 *    vector(i32) ints = null;
 *    vector_push(ints, 10);
 *    vector_push(ints, 3);
 *    vector_push(ints, 65);
 *    for (u32 i = 0; i < vector_count(ints); i++) {
 *        printf("%d\n", ints[i]);
 *    }
 *    free_vector(ints);
 * */

#define vector_default_capacity 8

struct vector_header {
	usize count;
	usize capacity;
	u64 element_size;
};

#define vector(t_) t_*

#define vector_push(v_, e_) \
	do { \
		if (!(v_)) { \
			struct vector_header h_ = { \
				.count = 1, \
				.capacity = 8, \
				.element_size = sizeof(*(v_)) \
			}; \
			\
			(v_) = core_alloc(sizeof(struct vector_header) + h_.element_size * h_.capacity); \
			\
			memcpy((v_), &h_, sizeof(struct vector_header)); \
			\
			(v_) = (void*)((struct vector_header*)(v_) + 1); \
			(v_)[0] = (e_); \
		} else { \
			struct vector_header* h_ = ((struct vector_header*)(v_)) - 1; \
			\
			if (h_->count >= h_->capacity) { \
				h_->capacity = h_->capacity * 2; \
				(v_) = core_realloc(h_, sizeof(struct vector_header) + h_->element_size * h_->capacity); \
				h_ = (struct vector_header*)(v_); \
				(v_) = (void*)(h_ + 1); \
			} \
			\
			(v_)[h_->count++] = (e_); \
		} \
	} while (0)

#define vector_allocate(v_, c_) \
	do { \
		if (c_ > 0) { \
			if (!(v_)) { \
				struct vector_header h_ = { \
					.count = 0, \
					.capacity = (c_), \
					.element_size = sizeof(*(v_)) \
				}; \
				\
				(v_) = core_alloc(sizeof(struct vector_header) + h_.element_size * h_.capacity); \
				\
				memcpy((v_), &h_, sizeof(struct vector_header)); \
				\
				(v_) = (void*)((struct vector_header*)(v_) + 1); \
			} else { \
				struct vector_header* h_ = ((struct vector_header*)(v_)) - 1; \
				\
				if ((c_) > h_->capacity) { \
					h_->capacity = (c_); \
					(v_) = core_realloc(h_, sizeof(struct vector_header) + h_->element_size * h_->capacity); \
					h_ = (struct vector_header*)(v_); \
					(v_) = (void*)(h_ + 1); \
				} \
			} \
		} \
	} while (0)

#define vector_pop(v_) \
	(v_) + (((((struct vector_header*)(v_)) - 1)->count--) - 1)

#define vector_count(v_) ((v_) != null ? (((struct vector_header*)(v_)) - 1)->count : 0)

#define vector_clear(v_) \
	do { \
		if (v_) { \
			(((struct vector_header*)(v_)) - 1)->count = 0; \
		} \
	} while (0)

#define vector_delete(v_, idx_) \
	do { \
		if (v_) { \
			memmove((v_) + (idx_), (v_) + vector_count(v_) - 1, sizeof (*(v_))); \
			(((struct vector_header*)(v_)) - 1)->count--; \
		} \
	} while (0)

#define free_vector(v_) if ((v_)) { core_free(((struct vector_header*)(v_)) - 1); }

#define vector_start(v_) (v_)
#define vector_end(v_) ((v_) != null ? ((v_) + ((((struct vector_header*)(v_)) - 1)->count)) : ((v_) + 1))

/* Optional. */
#define optional(t_) struct { bool has_value; t_ value; }
#define optional_set(o_, v_) \
	do { \
		(o_).value = v_; \
		(o_).has_value = true; \
	} while (0)

#define optional_remove_value(o_) \
	(o).has_value = false;

/* Linked list that doesn't do any allocation.
 * Rather, it relies on the users data type containing
 * a `next' and `prev' field.
 *
 * Lists should be initialised to null before usage. */

#define list(t_) struct { t_* head; t_* tail; }

#define list_insert(l_, n_, a_) \
	do { \
		if ((n_)) { \
			(a_)->next = (n_)->next; \
			(a_)->prev = (n_); \
			if ((n_)->next) { \
				(n_)->next->prev = (a_); \
			} \
			(n_)->next = (a_); \
		} else { \
			if (!(l_).head) { \
				(l_).head = (a_); \
				(l_).head->next = null; \
				(l_).head->prev = null; \
			} else { \
				(l_).head->prev = (a_); \
				(a_)->next = (l_).head; \
				(l_).head = (a_); \
			} \
		} \
		if (!(l_).tail) { \
			(l_).tail = (a_); \
			(l_).tail->next = null; \
			(l_).tail->prev = null; \
		} \
		if ((n_) == (l_).tail) { \
			(l_).tail = (a_); \
		} \
	} while (0)

#define list_push(l_, a_) \
	list_insert(l_, (l_).tail, a_)

#define list_remove(l_, n_) \
	do { \
		if (!(l_).head || !(n_)) { return; } \
		if ((l_).head == (n_)) { \
			(l_).head = (n_)->next; \
		} \
		if ((l_).tail == (n_)) { \
			(l_).tail = (n_)->prev; \
		} \
		if ((n_)->next) { \
			(n_)->next->prev = (n_)->prev; \
		} \
		if ((n_)->prev) { \
			(n_)->prev->next = (n_)->next; \
		} \
	} while (0)

/* RNG */
force_inline f32 random_float(f32 min, f32 max) {
	f32 scale = (f32)rand() / (f32)RAND_MAX;
	return min + scale * (max - min);
}

force_inline i32 random_int(i32 min, i32 max) {
	return (i32)random_float((f32)min, (f32)(max + 1));
}
