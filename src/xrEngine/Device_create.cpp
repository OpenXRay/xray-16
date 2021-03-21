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

extern XRCDB_API bool* cdb_bDebug;

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

    // Start all threads
    mt_bMustExit = false;

    CreateInternal();
}

void CRenderDevice::CreateInternal()
{
    if (b_is_Ready)
        return; // prevent double call

    Statistic = xr_new<CStats>();
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

    UpdateWindowProps(!psDeviceFlags.is(rsFullscreen));
    GEnv.Render->Create(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    Memory.mem_compact();
    b_is_Ready = true;

    _SetupStates();
    string_path fname;
    FS.update_path(fname, "$game_data$", "shaders.xr");
    GEnv.Render->OnDeviceCreate(fname);
    Statistic->OnDeviceCreate();
    dwFrame = 0;
    PreCache(0, false, false);
}

bool windowIntersectsWithMonitor(const SDL_Rect& window, const SDL_Rect& monitor)
{
    const int x = std::max(window.x, monitor.x);
    const int num1 = std::min(window.w, monitor.w);
    const int y = std::max(window.y, monitor.y);
    const int num2 = std::min(window.y, monitor.h);

    return num1 >= x && num2 >= y;
}

void CRenderDevice::UpdateWindowProps(const bool windowed)
{
    SelectResolution(windowed);

    if (windowed)
    {
        static bool drawBorders = strstr(Core.Params, "-draw_borders");

        bool useDesktopFullscreen = false;
        if (b_is_Ready)
        {
            if (!drawBorders && g_monitors.SelectedResolutionIsMaximal())
                useDesktopFullscreen = true;
        }

        SDL_Rect rect;
        SDL_GetDisplayBounds(Vid_SelectedMonitor, &rect);
        
        if (!windowIntersectsWithMonitor(m_rcWindowBounds, rect))
            SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);

        SDL_SetWindowBordered(m_sdlWnd, drawBorders ? SDL_TRUE : SDL_FALSE);
        SDL_SetWindowSize(m_sdlWnd, psCurrentVidMode[0], psCurrentVidMode[1]);

        // Set SDL_WINDOW_FULLSCREEN_DESKTOP if maximal resolution is selected
        SDL_SetWindowFullscreen(m_sdlWnd, useDesktopFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_DISABLE);
    }
    else
    {
        // Changing monitor, unset fullscreen for the previous monitor
        if (SDL_GetWindowDisplayIndex(m_sdlWnd) != Vid_SelectedMonitor)
            SDL_SetWindowFullscreen(m_sdlWnd, SDL_DISABLE);

        SDL_Rect rect;
        SDL_GetDisplayBounds(Vid_SelectedMonitor, &rect);
        SDL_SetWindowSize(m_sdlWnd, psCurrentVidMode[0], psCurrentVidMode[1]);
        SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);

        if (b_is_Ready)
        {
            SDL_SetWindowFullscreen(m_sdlWnd, SDL_WINDOW_FULLSCREEN);

            SDL_DisplayMode mode;
            SDL_GetWindowDisplayMode(m_sdlWnd, &mode);
            mode.w = psCurrentVidMode[0];
            mode.h = psCurrentVidMode[1];
            mode.refresh_rate = Vid_SelectedRefreshRate;
            SDL_SetWindowDisplayMode(m_sdlWnd, &mode);
        }
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

        if (!g_monitors.SelectedResolutionIsSafe()) // not found
        { // select safe
            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(Vid_SelectedMonitor, &current);
            current.w = psCurrentVidMode[0];
            current.h = psCurrentVidMode[1];

            SDL_DisplayMode closest; // try closest mode
            if (SDL_GetClosestDisplayMode(Vid_SelectedMonitor, &current, &closest))
                xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", closest.w, closest.h);
            else // or just use maximal
            {
                const auto& r = g_monitors.GetMaximalResolution();
                xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", r.first, r.second);
            }

            Console->Execute(buff);
        }

        if (!g_monitors.SelectedRefreshRateIsSafe())
        {
            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(Vid_SelectedMonitor, &current);
            current.refresh_rate = Vid_SelectedRefreshRate;

            SDL_DisplayMode closest; // try closest mode
            if (SDL_GetClosestDisplayMode(Vid_SelectedMonitor, &current, &closest))
                xr_sprintf(buff, sizeof(buff), "vid_refresh %d", closest.refresh_rate);
            else // or just use maximal
            {
                const u32 rate = g_monitors.GetMaximalRefreshRate();
                xr_sprintf(buff, sizeof(buff), "vid_refresh %d", rate);
            }

            Console->Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
}

SDL_Window* CRenderDevice::GetApplicationWindow()
{
    return m_sdlWnd;
}

void CRenderDevice::DisableFullscreen()
{
    SDL_SetWindowFullscreen(m_sdlWnd, SDL_FALSE);
}

void CRenderDevice::ResetFullscreen()
{
    UpdateWindowProps(!psDeviceFlags.test(rsFullscreen));
}
