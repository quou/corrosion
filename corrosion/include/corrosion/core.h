#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

/* Take the pointer of a literal. */
#define lit_ptr(l_) \
	_Generic((l_), \
		i8:  &(i8)  { l_ }, \
		i16: &(i16) { l_ }, \
		i32: &(i32) { l_ }, \
		i64: &(i64) { l_ }, \
		u8:  &(u8)  { l_ }, \
		u16: &(u16) { l_ }, \
		u32: &(u32) { l_ }, \
		u64: &(u64) { l_ }, \
		f32: &(f32) { l_ }, \
		f64: &(f64) { l_ }, \
		default: l_ \
	)

u64 elf_hash(const u8* data, usize size);
u64 hash_string(const char* str);

void info(const char* fmt, ...);
void error(const char* fmt, ...);
void warning(const char* fmt, ...);

void vinfo(const char* fmt, va_list args);
void verror(const char* fmt, va_list args);
void vwarning(const char* fmt, va_list args);

#define abort_with(fmt_, ...) \
	do { \
		error(fmt_, #__VA_ARGS__); \
		abort(); \
	} while (0)

void* core_alloc(usize size);
void* core_calloc(usize count, usize size);
void* core_realloc(void* ptr, usize size);
void core_free(void* ptr);

usize core_get_memory_usage();

struct type_info {
	u64 id;
	usize size;
	const char* name;
};

#define type_info(t_) \
	((struct type_info) { \
		.id = elf_hash((const u8*)#t_, sizeof(t_)), \
		.size = sizeof(t_), \
		.name = #t_ \
	})

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

#define table_null_key UINT64_MAX
#define table_load_factor 0.75

/* Takes a struct value instead of a type and gets
 * the offset of a property. */
#define voffsetof(v_, m_) \
	(usize)(((u8*)(&((v_).m_))) - ((u8*)(&(v_))))

enum {
	table_el_state_active = 0,
	table_el_state_inactive
};

#define _table_el(kt_, vt_) \
	struct { \
		kt_ key; \
		vt_ value; \
		u8 state; \
	}

/* Internal table functions, to be called upon by the table macros. */
void* _find_table_el(void* els, usize el_size, usize capacity, usize key_size, void* key,
	usize key_off, usize val_off, usize state_off, usize* ind);
void* _table_get(void* els, usize el_size, usize capacity, usize count, usize key_size, void* key,
	usize key_off, usize val_off, usize state_off);
void* _table_first_key(void* els, usize el_size, usize capacity, usize count, usize key_size,
	usize key_off, usize val_off, usize state_off);
void* _table_next_key(void* els, usize el_size, usize capacity, usize count, usize key_size, void* key,
	usize key_off, usize val_off, usize state_off);

#define _table_resize(t_, cap_) \
	do { \
		u64 nk_ = table_null_key; \
		u8* els_ = core_alloc((cap_) * sizeof (t_).e); \
		for (usize i = 0; i < (cap_); i++) { \
			memcpy(&els_[i * sizeof (t_).e + voffsetof((t_).e, key)], &nk_, sizeof (t_).k); \
			els_[i * sizeof (t_).e + voffsetof((t_).e, state)] = table_el_state_active; \
		} \
		\
		for (usize i_ = 0; i_ < (t_).capacity; i_++) { \
			u8* el_ = (((u8*)(t_).entries) + i_ * sizeof (t_).e); \
			if (memcmp(el_, &nk_, sizeof (t_).k) == 0) { continue; } \
			u8* dst_ = _find_table_el(els_, sizeof *(t_).entries, (t_).capacity, sizeof (t_).k, \
				el_ + voffsetof(*(t_).entries, key),\
				voffsetof(*(t_).entries, key), voffsetof(*(t_).entries, value), voffsetof((t_).e, state), \
				null); \
			memcpy(dst_ + voffsetof((t_).e, key),   el_ + voffsetof((t_).e, key),   sizeof (t_).k); \
			memcpy(dst_ + voffsetof((t_).e, value), el_ + voffsetof((t_).e, value), sizeof (t_).v); \
		} \
		if ((t_).entries) { core_free((t_).entries); } \
		(t_).entries = (void*)els_; \
		(t_).capacity = (cap_); \
	} while (0)

#define table(kt_, vt_) \
	struct { \
		_table_el(kt_, vt_)* entries; \
		usize count, capacity; \
		\
		/* Temporary element, key and value for assigning to
		 * take a pointer to to allow literals to be passed
		 * into the macros. */\
		_table_el(kt_, vt_) e; \
		kt_ k; \
		vt_ v; \
	}

#define free_table(t_) \
	do { \
		if ((t_).entries) { \
			core_free((t_).entries); \
		} \
	} while (0)

#define table_set(t_, k_, v_) \
	do { \
		if ((t_).count >= (t_).capacity * table_load_factor) { \
			usize new_cap_ = (t_).capacity < 8 ? 8 : (t_).capacity * 2; \
			_table_resize((t_), new_cap_); \
		} \
		(t_).k = (k_); \
		u64 nk_ = table_null_key; \
		u8* el_ = _find_table_el((t_).entries, sizeof *(t_).entries, (t_).capacity, sizeof (t_).k, \
			&(t_).k,\
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), null); \
		if (memcmp(el_ + voffsetof((t_).e, key), &nk_, sizeof (t_).k) == 0) { \
			(t_).count++; \
		} \
		(t_).v = (v_); \
		memcpy(el_ + voffsetof((t_).e, key),   &(t_).k, sizeof (t_).k); \
		memcpy(el_ + voffsetof((t_).e, value), &(t_).v, sizeof (t_).v); \
	} while (0)

#define table_get(t_, k_) \
	((t_).k = (k_), \
		_table_get((t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			&(t_).k,\
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state)))

#define table_delete(t_, k_) \
	do { \
		if ((t_).count > 0) { \
			(t_).k = (k_); \
			u8* el_ = _find_table_el((t_).entries, sizeof *(t_).entries, (t_).capacity, sizeof (t_).k, \
				&(t_).k,\
				voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state), null); \
			u64 nk_ = table_null_key; \
			memcpy(el_ + voffsetof((t_).e, key), &nk_, sizeof (t_).k); \
			*(el_ + voffsetof((t_).e, state)) = table_el_state_inactive; \
			(t_).count--; \
		} \
	} while (0)

#define table_first(t_) \
	_table_first_key((t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state))

#define table_next(t_, k_) \
	((t_).k = (k_), \
		_table_next_key((t_).entries, sizeof *(t_).entries, (t_).capacity, (t_).count, sizeof (t_).k, \
			&(t_).k, \
			voffsetof((t_).e, key), voffsetof((t_).e, value), voffsetof((t_).e, state)))

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
	(v_) + ((((struct vector_header*)(v_)) - 1)->count--)

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

/* String manipulation. */
char* copy_string(const char* str);

/* Optional. */
#define optional(t_) struct { bool has_value; t_ value; }
#define optional_set(o_, v_) \
	do { \
		(o_).value = v_; \
		(o_).has_value = true; \
	} while (0)

#define optional_remove_value(o_) \
	(o).has_value = false;
