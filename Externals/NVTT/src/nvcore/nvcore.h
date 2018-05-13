// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_H
#define NV_CORE_H

// cmake config
#include <nvconfig.h>

// Function linkage
#if NVCORE_SHARED
#ifdef NVCORE_EXPORTS
#define NVCORE_API DLL_EXPORT
#define NVCORE_CLASS DLL_EXPORT_CLASS
#else
#define NVCORE_API DLL_IMPORT
#define NVCORE_CLASS DLL_IMPORT
#endif
#else // NVCORE_SHARED
#define NVCORE_API
#define NVCORE_CLASS
#endif // NVCORE_SHARED


// Platform definitions
#include "poshlib/posh.h"

// OS:
// NV_OS_WIN32
// NV_OS_WIN64
// NV_OS_MINGW
// NV_OS_CYGWIN
// NV_OS_LINUX
// NV_OS_UNIX
// NV_OS_DARWIN

#define NV_OS_STRING 	POSH_OS_STRING

#if defined POSH_OS_LINUX
#	define NV_OS_LINUX 1
#	define NV_OS_UNIX 1
#elif defined POSH_OS_CYGWIN32
#	define NV_OS_CYGWIN 1
#elif defined POSH_OS_MINGW
#	define NV_OS_MINGW 1
#	define NV_OS_WIN32 1
#elif defined POSH_OS_OSX
#	define NV_OS_DARWIN 1
#	define NV_OS_UNIX 1
#elif defined POSH_OS_UNIX
#	define NV_OS_UNIX 1
#elif defined POSH_OS_WIN32
#	define NV_OS_WIN32 1
#elif defined POSH_OS_WIN64
#	define NV_OS_WIN64 1
#else
#	error "Unsupported OS"
#endif


// CPUs:
// NV_CPU_X86
// NV_CPU_X86_64
// NV_CPU_PPC

#define NV_CPU_STRING 	POSH_CPU_STRING

#if defined POSH_CPU_X86_64
#	define NV_CPU_X86_64 1
#elif defined POSH_CPU_X86
#	define NV_CPU_X86 1
#elif defined POSH_CPU_PPC
#	define NV_CPU_PPC 1
#else
#	error "Unsupported CPU"
#endif


// Compiler:
// NV_CC_GNUC
// NV_CC_MSVC
// @@ NV_CC_MSVC6
// @@ NV_CC_MSVC7
// @@ NV_CC_MSVC8

#if defined POSH_COMPILER_GCC
#	define NV_CC_GNUC	1
#	define NV_CC_STRING "gcc"
#elif defined POSH_COMPILER_MSVC
#	define NV_CC_MSVC	1
#	define NV_CC_STRING "msvc"
#else
#	error "Unsupported compiler"
#endif


// Endiannes:
#define NV_LITTLE_ENDIAN 	POSH_LITTLE_ENDIAN
#define NV_BIG_ENDIAN		POSH_BIG_ENDIAN
#define NV_ENDIAN_STRING	POSH_ENDIAN_STRING


// Version string:
#define NV_VERSION_STRING \
	NV_OS_STRING "/" NV_CC_STRING "/" NV_CPU_STRING"/" \
	NV_ENDIAN_STRING"-endian - " __DATE__ "-" __TIME__


/// Disable copy constructor and assignment operator. 
/// @hideinitializer
#define NV_FORBID_COPY(C) \
    private: \
    C( const C & ); \
    C &operator=( const C & );


/// Disable dynamic allocation on the heap. 
/// See Prohibiting Heap-Based Objects in More Effective C++.
/// @hideinitializer 
#define NV_FORBID_HEAPALLOC() \
	private: \
	static void *operator new(size_t size); \
	static void *operator new[](size_t size);

// String concatenation macros.
#define NV_STRING_JOIN2(arg1, arg2) NV_DO_STRING_JOIN2(arg1, arg2)
#define NV_DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define NV_STRING_JOIN3(arg1, arg2, arg3) NV_DO_STRING_JOIN3(arg1, arg2, arg3)
#define NV_DO_STRING_JOIN3(arg1, arg2, arg3) arg1 ## arg2 ## arg3

// Startup initialization macro.
#define NV_AT_STARTUP(some_code) \
	namespace { \
		static struct NV_STRING_JOIN2(AtStartup_, __LINE__) { \
			NV_STRING_JOIN2(AtStartup_, __LINE__)() { some_code; } \
		} \
		NV_STRING_JOIN3(AtStartup_, __LINE__, Instance); \
	};

/// Indicate the compiler that the parameter is not used to suppress compier warnings.
/// @hideinitializer 
#define NV_UNUSED(a) ((a)=(a))

/// Null index. @@ Move this somewhere else... This could have collisions with other definitions!
#define NIL uint(~0)

/// Null pointer.
#ifndef NULL
#define NULL 0
#endif

// Platform includes
#if NV_CC_MSVC
#	if NV_OS_WIN32
#		include "DefsVcWin32.h"
#	else
#		error "MSVC: Platform not supported"
#	endif
#elif NV_CC_GNUC
#	if NV_OS_LINUX
#		include "DefsGnucLinux.h"
#	elif NV_OS_DARWIN
#		include "DefsGnucDarwin.h"
#	elif NV_OS_MINGW
#		include "DefsGnucWin32.h"
#	elif NV_OS_CYGWIN
#		error "GCC: Cygwin not supported"
#	else
#		error "GCC: Platform not supported"
#	endif
#endif

#endif // NV_CORE_H
