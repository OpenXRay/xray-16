#pragma once

#if defined(_WIN32)
#   define XR_PLATFORM_WINDOWS
#   define _XRAY_PLATFORM_MARKER "Windows"
#elif defined(__linux__)
#   define XR_PLATFORM_LINUX
#   define _XRAY_PLATFORM_MARKER "Linux"
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(BSD)
#   define XR_PLATFORM_BSD
#   if defined(__FreeBSD__)
#       define XR_PLATFORM_FREEBSD
#       define _XRAY_PLATFORM_MARKER "FreeBSD"
#   elif defined(__OpenBSD__)
#       define XR_PLATFORM_OPENBSD
#       define _XRAY_PLATFORM_MARKER "OpenBSD"
#   elif defined(__NetBSD__)
#       define XR_PLATFORM_NETBSD
#       define _XRAY_PLATFORM_MARKER "NetBSD"
#   elif defined(__DragonFly__)
#       define XR_PLATFORM_DRAGONFLYBSD
#       define _XRAY_PLATFORM_MARKER "DragonFlyBSD"
#   else
#       define _XRAY_PLATFORM_MARKER "*BSD"
#   endif
#elif defined(SDL_PLATFORM_APPLE)
#   define XR_PLATFORM_APPLE
#   define _XRAY_PLATFORM_MARKER "Apple"
#else
#   error Unsupported platform
#endif

#if defined(_M_IX86) || defined(__i386__) || defined(_X86_)
#   define XR_ARCHITECTURE_X86
#   define _XRAY_ARCHITECTURE_MARKER "32-bit"
#elif defined(_M_X64) || defined(__amd64__) || defined(__x86_64__)
#   define XR_ARCHITECTURE_X64
#   define _XRAY_ARCHITECTURE_MARKER "64-bit"
#elif defined(_M_ARM) || defined(__arm__)
#   define XR_ARCHITECTURE_ARM
#   define _XRAY_ARCHITECTURE_MARKER "ARM 32-bit"
#elif defined (_M_ARM64) || defined(__aarch64__)
#   define XR_ARCHITECTURE_ARM64
#   define _XRAY_ARCHITECTURE_MARKER "ARM 64-bit"
#elif defined(__powerpc64__) || defined(__ppc64__)
#   define XR_ARCHITECTURE_PPC64
#   define _XRAY_ARCHITECTURE_MARKER "PowerPC 64-bit"
#elif defined (_M_PPC) || defined(__powerpc__)
#   define XR_ARCHITECTURE_PPC
#   define _XRAY_ARCHITECTURE_MARKER "PowerPC 32-bit"
#elif defined (__e2k__)
#   define XR_ARCHITECTURE_E2K
#   define _XRAY_ARCHITECTURE_MARKER "E2K"
#else
#   error Unsupported architecture
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
#elif defined(XR_PLATFORM_BSD)
#include "Common/PlatformBSD.inl"
#elif defined(XR_PLATFORM_APPLE)
#include "Common/PlatformApple.inl"
#else
#error Provide Platform.inl file for your platform
#endif

#ifdef XRAY_STATIC_BUILD
#   define _XRAY_STATIC_BUILD_MARKER "static"
#else
#   define _XRAY_STATIC_BUILD_MARKER "shared"
#endif

#ifdef MASTER_GOLD
#   define _XRAY_MASTER_GOLD_MARKER " Master Gold"
#else
#   define _XRAY_MASTER_GOLD_MARKER
#endif

#if defined(NDEBUG)
#   define _XRAY_CONFIGURATION_MARKER "Release"
#elif defined(MIXED)
#   define _XRAY_CONFIGURATION_MARKER "Mixed"
#elif defined(DEBUG)
#   define _XRAY_CONFIGURATION_MARKER "Debug"
#else
#   error Unknown configuration
#endif

#define XRAY_BUILD_CONFIGURATION _XRAY_CONFIGURATION_MARKER _XRAY_MASTER_GOLD_MARKER
#define XRAY_BUILD_CONFIGURATION2 _XRAY_PLATFORM_MARKER " " _XRAY_ARCHITECTURE_MARKER ", " _XRAY_STATIC_BUILD_MARKER
