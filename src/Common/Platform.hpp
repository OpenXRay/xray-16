#ifndef XRCORE_PLATFORM_H
#define XRCORE_PLATFORM_H
#pragma once

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#ifndef STRICT
# define STRICT // Enable strict syntax
#endif // STRICT
#define IDIRECTPLAY2_OR_GREATER // ?
#define DIRECTINPUT_VERSION 0x0800 //
#define _CRT_SECURE_NO_DEPRECATE // vc8.0 stuff, don't deprecate several ANSI functions

// windows.h
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0501
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

#include <ctime>
#define ALIGN(a) __declspec(align(a))
#include <sys\utime.h>

#define NOGDICAPMASKS
//#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NODRAWTEXT
#define NOMEMMGR
#define NOMETAFILE
#define NOSERVICE
#define NOCOMM
#define NOHELP
#define NOPROFILER
#define NOMCX
#define NOMINMAX
#define DOSWIN32
#define _WIN32_DCOM

#pragma warning(push)
#pragma warning(disable:4005)
#include <windows.h>
#ifndef __BORLANDC__
#include <windowsx.h>
#endif
#pragma warning(pop)

#endif