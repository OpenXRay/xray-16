// xrLC_Light.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "xrLc_globaldata.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

b_params& g_params()
{
    VERIFY(inlc_global_data());
    return inlc_global_data()->g_params();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        //xrDebug::Initialize(false);
        //bool init_log = (0 != xr_strcmp(Core.ApplicationName, "XRayEditorTools"));
        //Core.Initialize("xrLC_Light", 0, FALSE);
        //if (init_log)
            //CreateLog();

        // FPU::m64r ();
        break;
    }
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    case DLL_PROCESS_DETACH:
        if (inlc_global_data())
            destroy_global_data();
        //Core._destroy();
        break;
    }
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
