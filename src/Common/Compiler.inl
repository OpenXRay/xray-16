#if !defined(__GNUC__) && !defined(_MSC_VER)
#error Unsupported compiler
#endif

#ifdef _MSC_VER
#include <intrin.h> // for __debugbreak
#endif

#include "xr_impexp_macros.h"
#include "xrCommon/inlining_macros.h"

#if defined(__GNUC__)
#define XR_ASSUME(expr)  if (expr){} else __builtin_unreachable()
#elif defined(_MSC_VER)
#define XR_ASSUME(expr) __assume(expr)
#endif

#define UNUSED(...) (void)(__VA_ARGS__)
