#pragma once
#ifndef INLINING_MACROS_H
#define INLINING_MACROS_H

#if defined(__GNUC__)
#define NO_INLINE __attribute__((noinline))
#define FORCE_INLINE __attribute__((always_inline)) inline
#define ALIGN(a) __attribute__((aligned(a)))
#define DEBUG_BREAK asm("int $3")
#elif defined(_MSC_VER)
#define NO_INLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#define ALIGN(a) __declspec(align(a))
#define DEBUG_BREAK __debugbreak()
#define __thread __declspec(thread)
#endif

// XXX: remove
#define _inline inline
#define __inline inline

// XXX: remove IC/ICF/ICN
#define IC inline
#define ICF FORCE_INLINE
#define ICN NO_INLINE

#endif
