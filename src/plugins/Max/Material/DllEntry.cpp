#include "stdafx.h"
#pragma hdrstop

#include "plugins/Shared/GameMaterial.h"

extern ClassDesc2* GetXRayMtlDesc();
extern TCHAR* GetString(int id);

HINSTANCE hInstance;
int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    hInstance = hinstDLL; // Hang on to this DLL's instance handle.

    if (!controlsInit)
    {
        controlsInit = TRUE;
        Core.Initialize("S.T.A.L.K.E.R.Plugin", nullptr, 0, FALSE, nullptr, true);
        FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");
        FPU::m64r(); // нужно чтобы макс не сбрасывал контрольки в 0
        InitCustomControls(hInstance); // Initialize MAX's custom controls
        InitCommonControls(); // Initialize Win95 controls
        // load shader list
        XRayMtl::LoadXRayShaderList();
    }

    if (DLL_PROCESS_DETACH == fdwReason)
    {
        Core._destroy();
        XRayMtl::UnloadXRayShaderList();
    }

    return (TRUE);
}

__declspec(dllexport) const TCHAR* LibDescription() { return GetString(IDS_LIBDESCRIPTION); }
__declspec(dllexport) int LibNumberClasses() { return 1; }
__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
    switch (i)
    {
    case 0: return GetXRayMtlDesc();
    default: return 0;
    }
}

__declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
TCHAR* GetString(int id)
{
    static TCHAR buf[256];

    if (hInstance)
        return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

    return NULL;
}
