#include "stdafx.h"

#include "ETools.h"

#pragma warning(disable : 4267)

BOOL APIENTRY DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        //xrDebug::Initialize();
        //Core.Initialize("XRayEditorTools", 0, FALSE);
        // FPU::m64r	();
        break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    case DLL_PROCESS_DETACH: break;//Core._destroy(); break;
    }
    return TRUE;
}
