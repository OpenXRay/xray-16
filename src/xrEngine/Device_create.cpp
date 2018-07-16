#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCDB/xrXRC.h"
#include <SDL.h>
#include <SDL_syswm.h>


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
    GEnv.Render->SetupStates();
}

void CRenderDevice::Create()
{
    if (b_is_Ready)
        return; // prevent double call
    Statistic = new CStats();
    bool gpuSW = !!strstr(Core.Params, "-gpu_sw");
    bool gpuNonPure = !!strstr(Core.Params, "-gpu_nopure");
    bool gpuRef = !!strstr(Core.Params, "-gpu_ref");
    GEnv.Render->SetupGPU(gpuSW, gpuNonPure, gpuRef);
    Log("Starting RENDER device...");
#ifdef _EDITOR
    psCurrentVidMode[0] = dwWidth;
    psCurrentVidMode[1] = dwHeight;
#endif
    fFOV = 90.f;
    fASPECT = 1.f;
    GEnv.Render->Create(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);
    UpdateWindowProps();
    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowClient.x, &m_rcWindowClient.y);
    int w = 0, h = 0;
    SDL_GetWindowSize(m_sdlWnd, &w, &h);
    m_rcWindowClient.w = m_rcWindowClient.x + w;
    m_rcWindowClient.h = m_rcWindowClient.y + h;
    Memory.mem_compact();
    b_is_Ready = TRUE;
    _SetupStates();
    string_path fname;
    FS.update_path(fname, "$game_data$", "shaders.xr");
    GEnv.Render->OnDeviceCreate(fname);
    Statistic->OnDeviceCreate();
    dwFrame = 0;
    PreCache(0, false, false);
}

void CRenderDevice::UpdateWindowProps()
{
    const bool windowed = !psDeviceFlags.is(rsFullscreen) || editor();

    // Set window properties depending on what mode were in.
    if (windowed)
    {
        const bool drawBorders = strstr(Core.Params, "-draw_borders");
        if (drawBorders)
            SDL_SetWindowBordered(m_sdlWnd, SDL_TRUE);

        SDL_SetWindowSize(m_sdlWnd, psCurrentVidMode[0], psCurrentVidMode[1]);

        if (GEnv.isDedicatedServer || strstr(Core.Params, "-center_screen"))
            SDL_SetWindowPosition(m_sdlWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        else
        {
            int top = 0, left = 0, right = 0, bottom = 0;
            //SDL_GetWindowBordersSize(m_sdlWnd, &top, &left, &bottom, &right);
#ifdef WINDOWS
            // XXX: Currently SDL_GetWindowBordersSize is supported only on X11
            // For now we must use method below.
            if (drawBorders)
                top = GetSystemMetrics(SM_CYCAPTION); // size of the window title bar
#else
#pragma TODO("Implement for other platforms")
#endif
            SDL_SetWindowPosition(m_sdlWnd, left, top);
        }
    }

    if (!GEnv.isDedicatedServer)
        SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);
}
