#ifndef XRCORE_PLATFORM_H
#define XRCORE_PLATFORM_H
#pragma once

#define VC_EXTRALEAN				// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers
#ifndef STRICT
#	define STRICT					// Enable strict syntax
#endif // STRICT
#define IDIRECTPLAY2_OR_GREATER		// ?
#define DIRECTINPUT_VERSION	0x0800	//
#define _CRT_SECURE_NO_DEPRECATE	// vc8.0 stuff, don't deprecate several ANSI functions

// windows.h
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0500
#endif

#ifdef __BORLANDC__
	#include <vcl.h>
	#include <mmsystem.h>
	#include <stdint.h>
#endif

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