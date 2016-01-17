#pragma once
#include "Common/Config.hpp"

#if defined(__linux__)
#define LINUX
#elif defined(_WIN32)
#define WINDOWS
#else
#error Unsupported platform
#endif

#ifdef __GNUC__
#define XR_EXPORT __attribute__ ((visibility("default")))
#define XR_IMPORT __attribute__ ((visibility("default")))
#else // _MSC_VER
#define XR_EXPORT __declspec(dllexport)
#define XR_IMPORT __declspec(dllimport)
#endif

// inline control - redefine to use compiler's heuristics ONLY
// it seems "IC" is misused in many places which cause code-bloat
// ...and VC7.1 really don't miss opportunities for inline :)
#define _inline inline
#define __inline inline
#define IC inline
#ifdef _EDITOR
# define ICF inline
# define ICN
#else
# define ICF __forceinline // !!! this should be used only in critical places found by PROFILER
# define ICN __declspec (noinline)
#endif
#define ALIGN(a) __declspec(align(a))

#include <ctime>
#include <sys\utime.h>

#if defined(LINUX)
#include "Common/PlatformLinux.inl"
#elif defined(WINDOWS)
#include "Common/PlatformWindows.inl"
#endif
