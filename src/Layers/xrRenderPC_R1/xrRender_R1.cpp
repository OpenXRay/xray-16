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
	case DLL_PROCESS_ATTACH:
        SetupEnv();
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
