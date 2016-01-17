#include "stdafx.h"
#include <d3d9types.h>
#include <d3d9caps.h>
#include "IDirect3D9.h"

#include "Common/Platform.hpp"

#ifdef XRD3D9NULL_EXPORTS
#define XRD3D9NULL_API XR_EXPORT
#else
#define XRD3D9NULL_API XR_IMPORT
#endif



extern "C" {
	 IDirect3D9* WINAPI  Direct3DCreate9(UINT SDKVersion);
};



