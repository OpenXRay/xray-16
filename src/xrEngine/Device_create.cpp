#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCDB/xrXRC.h"

void CRenderDevice::SetupStates()
{
    // General Render States
    mView.identity();
    mProject.identity();
    mFullTransform.identity();
    vCameraPosition.set(0, 0, 0);
    vCameraDirection.set(0, 0, 1);
    vCameraTop.set(0, 1, 0);
    vCameraRight.set(1, 0, 0);
    GEnv.Render->SetupStates();
}

void CRenderDevice::Create()
{
    if (b_is_Ready)
        return; // prevent double call

    ZoneScoped;

    Statistic = xr_new<CStats>();
    Log("Starting RENDER device...");
#ifdef _EDITOR
    psDeviceMode.Width = dwWidth;
    psDeviceMode.Height = dwHeight;
#endif
    fFOV = 90.f;
    fASPECT = 1.f;

    if (GEnv.isDedicatedServer)
        psDeviceMode.WindowStyle = rsWindowed;

    SyncWindowToPsDeviceMode();

    int pxW = 0, pxH = 0;
    SDL_GetWindowSizeInPixels(m_sdlWnd, &pxW, &pxH);
    if (pxW > 0)
        dwWidth = static_cast<u32>(pxW);
    if (pxH > 0)
        dwHeight = static_cast<u32>(pxH);

    GEnv.Render->Create(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    Memory.mem_compact();
    b_is_Ready = true;

    SetupStates();
    string_path fname;
    FS.update_path(fname, "$game_data$", "shaders.xr");
    GEnv.Render->OnDeviceCreate(fname);
    m_imgui_render = GEnv.RenderFactory->CreateImGuiRender();
    m_imgui_render->OnDeviceCreate(GetImGuiContext());
    //Statistic->OnDeviceCreate();
    dwFrame = 0;
}
