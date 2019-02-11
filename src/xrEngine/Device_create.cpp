#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCore/xr_token.h"
#include "xrCDB/xrXRC.h"
#include "XR_IOConsole.h"
#include "MonitorManager.hpp"
#include "SDL.h"	
#include "SDL_syswm.h"

extern u32 Vid_SelectedMonitor;
extern u32 Vid_SelectedRefreshRate;

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

    if (GEnv.isDedicatedServer || editor())
        psDeviceFlags.set(rsFullscreen, false);

    g_monitors.Initialize();
    UpdateWindowProps(!psDeviceFlags.is(rsFullscreen));
    GEnv.Render->Create(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

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

void CRenderDevice::UpdateWindowProps(const bool windowed)
{
    SelectResolution(windowed);

    if (windowed)
    {
        // Get the maximal available resolution (penultimate token in VidModesToken)
        const MonitorsManager::ResolutionPair r = g_monitors.GetMaximalResolution();

        const bool drawBorders = strstr(Core.Params, "-draw_borders");

        bool maximalResolution = false;
        if (b_is_Ready && !drawBorders && psCurrentVidMode[0] == r.first && psCurrentVidMode[1] == r.second)
            maximalResolution = true;

        // Set SDL_WINDOW_FULLSCREEN_DESKTOP if maximal resolution is selected
        SDL_SetWindowFullscreen(m_sdlWnd, maximalResolution ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_SetWindowSize(m_sdlWnd, psCurrentVidMode[0], psCurrentVidMode[1]);
        SDL_SetWindowBordered(m_sdlWnd, drawBorders ? SDL_TRUE : SDL_FALSE);
    }
    else
    {
        SDL_SetWindowFullscreen(m_sdlWnd, SDL_WINDOW_FULLSCREEN);

        // XXX: fix monitor selection
        // it appears to be buggy
        SDL_Rect rect;
        SDL_GetDisplayBounds(Vid_SelectedMonitor, &rect);
        SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);
        SDL_DisplayMode mode;
        SDL_GetWindowDisplayMode(m_sdlWnd, &mode);
        mode.w = psCurrentVidMode[0];
        mode.h = psCurrentVidMode[1];
        mode.refresh_rate = Vid_SelectedRefreshRate;
        SDL_SetWindowDisplayMode(m_sdlWnd, &mode);
    }

    UpdateWindowRects();
    SDL_FlushEvents(SDL_WINDOWEVENT, SDL_SYSWMEVENT);
}


void CRenderDevice::UpdateWindowRects()
{
    m_rcWindowClient.x = 0;
    m_rcWindowClient.y = 0;
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowClient.w, &m_rcWindowClient.h);

    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowBounds.x, &m_rcWindowBounds.y);
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowBounds.w, &m_rcWindowBounds.h);
    m_rcWindowBounds.w += m_rcWindowBounds.x;
    m_rcWindowBounds.h += m_rcWindowBounds.y;

#if SDL_VERSION_ATLEAST(2,0,5)
    // Do we need code below?
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(m_sdlWnd, &top, &left, &bottom, &right);
    m_rcWindowBounds.x -= left;
    m_rcWindowBounds.y -= top;
    m_rcWindowBounds.w += right;
    m_rcWindowBounds.h += bottom;
    // XXX: check if we need this code when SDL_GetWindowBordersSize
    // will be available for Windows
#endif
}

void CRenderDevice::SelectResolution(const bool windowed)
{
    // Dedicated server hardcoded resolution
    // XXX: to be removed
    if (GEnv.isDedicatedServer)
    {
        dwWidth = psCurrentVidMode[0] = 640;
        dwHeight = psCurrentVidMode[1] = 480;
    }
    else if (windowed)
    {
        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
    else // check
    {
        string32 buff;
        xr_sprintf(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

        if (g_monitors.SelectedResolutionIsSafe()) // not found
        { // select safe
            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(Vid_SelectedMonitor, &current);
            current.w = psCurrentVidMode[0];
            current.h = psCurrentVidMode[1];

            SDL_DisplayMode closest;
            if (SDL_GetClosestDisplayMode(Vid_SelectedMonitor, &current, &closest))
                xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", closest.w, closest.h);
            else
                xr_sprintf(buff, sizeof(buff), "vid_mode %s", g_monitors.GetMaximalResolution());

            Console->Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
}
