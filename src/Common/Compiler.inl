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

#ifdef _MSC_VER
// We use xr_* instead of defining e.g. strupr => _strupr, since the macro definition could
// come before the std. header file declaring it, and thereby renaming that one too.
#define xr_strupr _strupr
#define xr_strlwr _strlwr
#define xr_stricmp _stricmp
#define xr_strcmpi _strcmpi
#define xr_unlink _unlink
#define xr_itoa _itoa
#else
#define xr_strupr strupr
#define xr_strlwr strlwr
#define xr_stricmp stricmp
#define xr_strcmpi strcmpi
#define xr_unlink unlink
#define xr_itoa itoa
#endif
