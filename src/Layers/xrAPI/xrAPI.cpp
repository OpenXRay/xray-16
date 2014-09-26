// xrAPI.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "../../Include/xrApi/xrAPI.h"

XRAPI_API IRender_interface*	Render = NULL;
XRAPI_API IRenderFactory*	RenderFactory = NULL;
XRAPI_API CDUInterface*	DU = NULL;
XRAPI_API xr_token*	vid_mode_token = NULL;
XRAPI_API IUIRender* UIRender = NULL;
#ifndef	_EDITOR
//XRAPI_API CSound_manager_interface*	Sound = NULL;
XRAPI_API CGameMtlLibrary*	PGMLib = NULL;
#endif
#ifdef DEBUG
	XRAPI_API IDebugRender*	DRender = NULL;
#endif // DEBUG

/*
// This is an example of an exported variable
XRAPI_API int nxrAPI=0;

// This is an example of an exported function.
XRAPI_API int fnxrAPI(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see xrAPI.h for the class definition
CxrAPI::CxrAPI()
{
	return;
}
*/
