// xrRender_R2.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

#pragma comment(lib, "xrEngine.lib")
#pragma comment(lib, "xrScriptEngine.lib")

extern "C" void XR_EXPORT SetupEnv()
{
    GlobalEnv.Render = &RImplementation;
    GlobalEnv.RenderFactory = &RenderFactoryImpl;
    GlobalEnv.DU = &DUImpl;
    GlobalEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
    GlobalEnv.DRender = &DebugRenderImpl;
#endif
    xrRender_initconsole();
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH	:
        SetupEnv();
        break;
	case DLL_THREAD_ATTACH	:
	case DLL_THREAD_DETACH	:
	case DLL_PROCESS_DETACH	:
		break;
	}
	return TRUE;
}

extern "C"
{
	bool _declspec(dllexport) SupportsAdvancedRendering();
};

bool _declspec(dllexport) SupportsAdvancedRendering()
{
	D3DCAPS9					caps;
	CHW							_HW;
	_HW.CreateD3D				();
	_HW.pD3D->GetDeviceCaps		(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
	_HW.DestroyD3D				();
	u16		ps_ver_major		= u16 ( u32(u32(caps.PixelShaderVersion)&u32(0xf << 8ul))>>8 );

	if (ps_ver_major<3)
		return false;
	else
		return true;
}