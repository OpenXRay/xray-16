// xrD3D9-Null.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "xrD3D9-Null.h"
#include "xrD3D9-Null_OutProc.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
/*
// This is an example of an exported variable
XRD3D9NULL_API int nxrD3D9Null=0;

// This is an example of an exported function.
XRD3D9NULL_API int fnxrD3D9Null(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see xrD3D9-Null.h for the class definition
CxrD3D9Null::CxrD3D9Null()
{ 
	return; 
}
*/

 IDirect3D9 * WINAPI Direct3DCreate9(UINT SDKVersion)
{
	UINT cSDKVersion = D3D_SDK_VERSION;
//	LogOut_File("In %x out %x", cSDKVersion, SDKVersion);
//	LogOut("In %d out %d", cSDKVersion, SDKVersion);
#ifdef NDEBUG
	if (SDKVersion != cSDKVersion)
	{
//		LogOut_File("NULL");
		LogOut_File( "cSDKVersion = %d, SDKVersion = %d", cSDKVersion, SDKVersion );
		return NULL;
	}
#endif
	xrIDirect3D9* I = new xrIDirect3D9();
//	LogOut_File("%x", I);
	return I;
}

