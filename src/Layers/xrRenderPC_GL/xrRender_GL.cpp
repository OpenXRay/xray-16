// xrRender_GL.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "glRenderFactory.h"
#include "glUIRender.h"
#include "glDebugRender.h"

#pragma comment(lib,"xrEngine.lib")

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH	:
		//	Can't call CreateDXGIFactory from DllMain
		//if (!xrRender_test_hw())	return FALSE;
		::Render					= &RImplementation;
		::RenderFactory				= &RenderFactoryImpl;
		::DU						= &DUImpl;
		//::vid_mode_token			= filled by glRenderDeviceRender
		UIRender					= &UIRenderImpl;
#ifdef DEBUG
		DRender						= &DebugRenderImpl;
#endif	//	DEBUG
		xrRender_initconsole		();
		break	;
	case DLL_THREAD_ATTACH	:
	case DLL_THREAD_DETACH	:
	case DLL_PROCESS_DETACH	:
		break;
	}
	return TRUE;
}
