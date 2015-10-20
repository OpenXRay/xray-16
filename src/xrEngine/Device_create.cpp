#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "dedicated_server_only.h"
#include "xrCDB/xrXRC.h"
#include "securom_api.h"

extern XRCDB_API BOOL* cdb_bDebug;

void CRenderDevice::_SetupStates()
{
    // General Render States
    mView.identity();
    mProject.identity();
    mFullTransform.identity();
    vCameraPosition.set(0, 0, 0);
    vCameraDirection.set(0, 0, 1);
    vCameraTop.set(0, 1, 0);
    vCameraRight.set(1, 0, 0);
    Render->SetupStates();
}

void CRenderDevice::_Create(LPCSTR shName)
{
    Memory.mem_compact();
    // after creation
    b_is_Ready = TRUE;
    _SetupStates();
    Render->OnDeviceCreate(shName);
    Statistic->OnDeviceCreate();
    dwFrame = 0;
}

PROTECT_API void CRenderDevice::Create()
{
    SECUROM_MARKER_SECURITY_ON(4);
    if (b_is_Ready)
        return; // prevent double call
    Statistic = xr_new<CStats>();
    // XXX: delete
//#ifdef DEBUG
    //cdb_clRAY = &Statistic->clRAY; // total: ray-testing
    //cdb_clBOX = &Statistic->clBOX; // total: box query
    //cdb_clFRUSTUM = &Statistic->clFRUSTUM; // total: frustum query
    //cdb_bDebug = &bDebug;
//#endif
    bool gpuSW = !!strstr(Core.Params, "-gpu_sw");
    bool gpuNonPure = !!strstr(Core.Params, "-gpu_nopure");
    bool gpuRef = !!strstr(Core.Params, "-gpu_ref");
    Render->SetupGPU(gpuSW, gpuNonPure, gpuRef);
    Log("Starting RENDER device...");
#ifdef _EDITOR
    psCurrentVidMode[0] = dwWidth;
    psCurrentVidMode[1] = dwHeight;
#endif
    fFOV = 90.f;
    fASPECT = 1.f;
#ifdef INGAME_EDITOR
    bool noEd = !editor();
#else
    bool noEd = true;
#endif
    Render->Create(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2, noEd);
    string_path fname;
    FS.update_path(fname, "$game_data$", "shaders.xr");
    _Create(fname);
    PreCache(0, false, false);
    SECUROM_MARKER_SECURITY_OFF(4);
}
