// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the XRD3D9NULL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// XRD3D9NULL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef XRD3D9NULL_EXPORTS
#define XRD3D9NULL_API __declspec(dllexport)
#else
#define XRD3D9NULL_API __declspec(dllimport)
#endif
//---------------------------------
#include <stdlib.h>
#include <objbase.h>
#include <windows.h>
//---------------------------------
#include "d3d9types.h"
#include "d3d9caps.h"
//---------------------------------

#include "IDirect3D9.h"



extern "C" {
	 IDirect3D9* WINAPI  Direct3DCreate9(UINT SDKVersion);
};



