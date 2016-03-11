#if !defined(__GNUC__) && !defined(_MSC_VER)
#error Unsupported compiler
#endif

#ifdef _MSC_VER
#include <intrin.h> // for __debugbreak
#endif

#if defined(__GNUC__)
#define XR_EXPORT __attribute__ ((visibility("default")))
#define XR_IMPORT __attribute__ ((visibility("default")))
#elif defined(_MSC_VER)
#define XR_EXPORT __declspec(dllexport)
#define XR_IMPORT __declspec(dllimport)
#endif

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

#define UNUSED(...) (void)(__VA_ARGS__)
