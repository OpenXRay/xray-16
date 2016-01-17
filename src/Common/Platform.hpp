#pragma once
#include "Common/Config.hpp"

#if defined(__linux__)
#define LINUX
#elif defined(_WIN32)
#define WINDOWS
#else
#error Unsupported platform
#endif

#if defined(_M_X64) || defined(__amd64__)
#define XR_X64
#else
#define XR_X86
#endif

#include "Common/Compiler.inl"

#include <ctime>
#include <sys\utime.h>

#if defined(LINUX)
#include "Common/PlatformLinux.inl"
#elif defined(WINDOWS)
#include "Common/PlatformWindows.inl"
#endif
