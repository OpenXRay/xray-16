#include "stdafx.h"
#include <d3d9types.h>
#include <d3d9caps.h>
#include "IDirect3D9.h"


#ifdef XRD3D9NULL_EXPORTS
#define XRD3D9NULL_API __declspec(dllexport)
#else
#define XRD3D9NULL_API __declspec(dllimport)
#endif



extern "C" {
	 IDirect3D9* WINAPI  Direct3DCreate9(UINT SDKVersion);
};



