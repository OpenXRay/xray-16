#ifndef NV_CORE_H
#error "Do not include this file directly."
#endif

#include <stdint.h> // uint8_t, int8_t, ...

// Function linkage
#define DLL_IMPORT
#if __GNUC__ >= 4
#	define DLL_EXPORT __attribute__((visibility("default")))
#	define DLL_EXPORT_CLASS DLL_EXPORT
#else
#	define DLL_EXPORT
#	define DLL_EXPORT_CLASS
#endif

// Function calling modes
#if NV_CPU_X86
#	define NV_CDECL 	__attribute__((cdecl))
#	define NV_STDCALL	__attribute__((stdcall))
#else
#	define NV_CDECL 
#	define NV_STDCALL
#endif

#define NV_FASTCALL		__attribute__((fastcall))
#define NV_FORCEINLINE	__attribute__((always_inline))
#define NV_DEPRECATED   __attribute__((deprecated))

#if __GNUC__ > 2
#define NV_PURE     __attribute__((pure))
#define NV_CONST    __attribute__((const))
#else
#define NV_PURE
#define NV_CONST
#endif

// Define __FUNC__ properly.
#if __STDC_VERSION__ < 199901L
#	if __GNUC__ >= 2
#		define __FUNC__ __PRETTY_FUNCTION__	// __FUNCTION__
#	else
#		define __FUNC__ "<unknown>"
#	endif
#else
#	define __FUNC__ __PRETTY_FUNCTION__
#endif

#define restrict    __restrict__


// Type definitions
typedef uint8_t     uint8;
typedef int8_t      int8;

typedef uint16_t    uint16;
typedef int16_t     int16;

typedef uint32_t    uint32;
typedef int32_t     int32;

typedef uint64_t    uint64;
typedef int64_t     int64;

// Aliases
typedef uint32      uint;
