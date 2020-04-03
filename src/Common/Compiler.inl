#if defined(__GNUC__)
#define NO_INLINE               __attribute__((noinline))
#define FORCE_INLINE            __attribute__((always_inline)) inline
#define ALIGN(a)                __attribute__((aligned(a)))

// Debugger trap implementation
#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64)
#define DEBUG_BREAK             do { __asm__ volatile ("int $3"); } while(0)
#elif defined(XR_ARCHITECTURE_ARM)
#define DEBUG_BREAK             do { __asm__ volatile (".inst 0xe7f001f0"); } while(0)
#elif defined(XR_ARCHITECTURE_ARM64)
#define DEBUG_BREAK             do { __asm__ volatile (".inst 0xd4200000"); } while(0)
#elif __has_include(<signal.h>)
#include <signal.h>
#if defined(SIGTRAP)
#define DEBUG_BREAK             raise(SIGTRAP) // SIGTRAP is preferred
#else
#define DEBUG_BREAK             __builtin_trap() // raises SIGILL
#endif
#else
#define DEBUG_BREAK             __builtin_trap() // raises SIGILL
#endif

#elif defined(_MSC_VER)
#include <intrin.h> // for __debugbreak

#define NO_INLINE               __declspec(noinline)
#define FORCE_INLINE            __forceinline
#define ALIGN(a)                __declspec(align(a))
#define DEBUG_BREAK             __debugbreak()
#define __thread                __declspec(thread)
#else
#error Provide your definitions here
#endif

// XXX: remove IC/ICF/ICN
#define IC                      inline
#define ICF                     FORCE_INLINE
#define ICN                     NO_INLINE

#define UNUSED(...) (void)(__VA_ARGS__)

#if defined(__GNUC__)
#define XR_ASSUME(expr)  if (expr){} else __builtin_unreachable()

#define XR_EXPORT __attribute__ ((visibility("default")))
#define XR_IMPORT __attribute__ ((visibility("default")))
#elif defined(_MSC_VER)
#define XR_ASSUME(expr) __assume(expr)

#define XR_EXPORT __declspec(dllexport)
#define XR_IMPORT __declspec(dllimport)
#else
#error Provide your definitions here
#endif

#ifndef _CPPUNWIND//def NDEBUG
#define XR_NOEXCEPT throw()
#define XR_NOEXCEPT_OP(x)
#else
#define XR_NOEXCEPT noexcept
#define XR_NOEXCEPT_OP(x) noexcept(x)
#endif

// We use xr_* instead of defining e.g. strupr => _strupr, since the macro definition could
// come before the std. header file declaring it, and thereby renaming that one too.
#ifdef _MSC_VER
#define xr_alloca _alloca
#define xr_strupr _strupr
#define xr_strlwr _strlwr
#define xr_stricmp _stricmp
#define xr_strcmpi _strcmpi
#define xr_unlink _unlink
#define xr_itoa _itoa
#else
#if !defined(__INTEL_COMPILER)
#define xr_alloca alloca
#else
#define xr_alloca _alloca
#endif

#define xr_strupr strupr
#define xr_strlwr strlwr
#define xr_stricmp stricmp
#define xr_strcmpi strcmpi
#define xr_itoa itoa
#endif
