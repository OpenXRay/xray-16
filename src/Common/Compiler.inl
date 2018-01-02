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

#include "xrCommon/inlining_macros.h"

#if defined(__GNUC__)
#define XR_ASSUME(expr)  if (expr){} else __builtin_unreachable()
#elif defined(_MSC_VER)
#define XR_ASSUME(expr) __assume(expr)
#endif

#define UNUSED(...) (void)(__VA_ARGS__)

#ifndef _CPPUNWIND//def NDEBUG
#define XR_NOEXCEPT throw()
#define XR_NOEXCEPT_OP(x)
#else
#define XR_NOEXCEPT noexcept
#define XR_NOEXCEPT_OP(x) noexcept(x)
#endif
