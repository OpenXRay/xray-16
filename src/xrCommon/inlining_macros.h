#pragma once

#if defined(__GNUC__)
#define NO_INLINE __attribute__((noinline))
#define FORCE_INLINE __attribute__((always_inline)) inline
#define ALIGN(a) __attribute__((aligned(a)))
#define DEBUG_BREAK __builtin_trap()
#elif defined(_MSC_VER)
#define NO_INLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#define ALIGN(a) __declspec(align(a))
#define DEBUG_BREAK __debugbreak()
#define __thread __declspec(thread)
#endif

// XXX: remove IC/ICF/ICN
#define IC inline
#define ICF FORCE_INLINE
#define ICN NO_INLINE
