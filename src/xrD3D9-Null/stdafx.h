// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <d3d9.h>
#include <objbase.h>
#include <stdlib.h>
#include <windows.h>

#pragma warning(disable : 4996)
