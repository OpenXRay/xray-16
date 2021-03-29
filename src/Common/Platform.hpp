#pragma once

#if defined(_WIN32)
#define XR_PLATFORM_WINDOWS
#elif defined(__linux__)
#define XR_PLATFORM_LINUX
#elif defined(__FreeBSD__)
#define XR_PLATFORM_FREEBSD
#else
#error Unsupported platform
#endif

#if defined(_M_IX86) || defined(__i386__) || defined(_X86_)
#define XR_ARCHITECTURE_X86
#elif defined(_M_X64) || defined(__amd64__) || defined(__x86_64__)
#define XR_ARCHITECTURE_X64
#elif defined(_M_ARM) || defined(__arm__)
#define XR_ARCHITECTURE_ARM
#elif defined (_M_ARM64) || defined(__aarch64__)
#define XR_ARCHITECTURE_ARM64
#elif defined (__e2k__)
#define XR_ARCHITECTURE_E2K
#else
#error Unsupported architecture
#endif

#if defined(_MSC_VER)
#define XR_COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
#define XR_COMPILER_GCC __GNUC__
#else
#error Unsupported compiler
#endif
#include "Common/Compiler.inl"

#if defined(XR_PLATFORM_WINDOWS)
#include "Common/PlatformWindows.inl"
#elif defined(XR_PLATFORM_LINUX)
#include "Common/PlatformLinux.inl"
#elif defined(XR_PLATFORM_FREEBSD)
#include "Common/PlatformBSD.inl"
#else
#error Provide Platform.inl file for your platform
#endif
