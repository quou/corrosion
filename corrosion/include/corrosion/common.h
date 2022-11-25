#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uptr;

typedef float  f32;
typedef double f64;

typedef size_t usize;

#if defined(__cplusplus)
	#define null nullptr
#else
	#define null ((void*)0x0)
#endif

#if defined(__clang__) || defined(__gcc__)
	#define force_inline __attribute__((always_inline)) inline
	#define dont_inline __attribute__((noinline))
#elif defined(_MSC_VER)
	#define force_inline __forceinline
	#define dont_inline __declspec(noinline)
#else
	#define force_inline static inline
	#define dont_inline
#endif

