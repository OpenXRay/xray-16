#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCore/xr_token.h"
#include "xrCDB/xrXRC.h"
#include "XR_IOConsole.h"
#include <SDL.h>
#include <SDL_syswm.h>

extern u32 Vid_SelectedMonitor;
extern u32 Vid_SelectedRefreshRate;
extern xr_vector<xr_token> VidMonitorsToken;
extern xr_vector<xr_token> VidModesToken;
extern xr_vector<xr_token> VidRefreshRateToken;

extern XRCDB_API BOOL* cdb_bDebug;

void FillMonitorsToken();
void FreeMonitorsToken();

void FillVidModesToken(u32 monitorID);
void FreeVidModesToken();

void FillRefreshRateToken();
void FreeRefreshRateToken();

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

    FillVidModesToken(Vid_SelectedMonitor);
    SelectResolution(!psDeviceFlags.is(rsFullscreen));
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

void CRenderDevice::UpdateWindowProps(bool windowed)
{
    SelectResolution(windowed);

    SDL_SetWindowFullscreen(m_sdlWnd, windowed ? 0 : SDL_WINDOW_FULLSCREEN);
    SDL_Rect rect;

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
            SDL_GetDisplayUsableBounds(Vid_SelectedMonitor, &rect);

            int top = 0, left = 0, right = 0, bottom = 0;
            // SDL_GetWindowBordersSize(m_sdlWnd, &top, &left, &bottom, &right);
#ifdef WINDOWS
            // XXX: Currently SDL_GetWindowBordersSize is supported only on X11
            // For now we must use method below.
            if (drawBorders)
                top = GetSystemMetrics(SM_CYCAPTION); // size of the window title bar
#else
#pragma TODO("Implement for other platforms")
#endif
            SDL_SetWindowPosition(m_sdlWnd, rect.x + left, rect.y + top);
        }

        SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowClient.x, &m_rcWindowClient.y);
        int w = 0, h = 0;
        SDL_GetWindowSize(m_sdlWnd, &w, &h);
        m_rcWindowClient.w = m_rcWindowClient.x + w;
        m_rcWindowClient.h = m_rcWindowClient.y + h;
    }
    else
    {
        SDL_GetDisplayBounds(Vid_SelectedMonitor, &rect);
        SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);
        SDL_DisplayMode mode;
        SDL_GetWindowDisplayMode(m_sdlWnd, &mode);
        mode.w = psCurrentVidMode[0];
        mode.h = psCurrentVidMode[1];
        mode.refresh_rate = Vid_SelectedRefreshRate;
        SDL_SetWindowDisplayMode(m_sdlWnd, &mode);
    }

    if (!GEnv.isDedicatedServer)
        SDL_SetWindowGrab(m_sdlWnd, SDL_TRUE);

    UpdateWindowRect();
}


void CRenderDevice::UpdateWindowRect()
{
    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowClient.x, &m_rcWindowClient.y);
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowClient.w, &m_rcWindowClient.h);
    m_rcWindowClient.w += m_rcWindowClient.x;
    m_rcWindowClient.h += m_rcWindowClient.y;
}

void CRenderDevice::SelectResolution(bool windowed)
{
    if (GEnv.isDedicatedServer)
    {
        dwWidth = 640;
        dwHeight = 480;
        return;
    }

    if (windowed)
    {
        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
    else // check
    {
        string32 buff;
        xr_sprintf(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

        if (_ParseItem(buff, VidModesToken.data()) == u32(-1)) // not found
        { // select safe
            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(Vid_SelectedMonitor, &current);
            current.w = psCurrentVidMode[0];
            current.h = psCurrentVidMode[1];

            SDL_DisplayMode closest;
            if (SDL_GetClosestDisplayMode(Vid_SelectedMonitor, &current, &closest))
                xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", closest.w, closest.h);
            else
                xr_sprintf(buff, sizeof(buff), "vid_mode %s", VidModesToken[0].name);

            Console->Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
}

struct uniqueRenderingMode
{
    uniqueRenderingMode(pcstr v) : value(v) {}
    pcstr value;
    bool operator()(const xr_token& other) const
    {
        return !xr_stricmp(value, other.name);
    }
};

void FillMonitorsToken()
{
    auto& monitors = VidMonitorsToken;

    if (!monitors.empty())
        FreeMonitorsToken();

    int displayCount = SDL_GetNumVideoDisplays();
    R_ASSERT3(displayCount > 0, "Failed to find display", SDL_GetError());
    monitors.reserve(displayCount + 1);

    for (int i = 0; i < displayCount; ++i)
        monitors.emplace_back(xr_strdup(SDL_GetDisplayName(i)), i);

    monitors.emplace_back(nullptr, -1);
}

void FillVidModesToken(u32 monitorID)
{
    auto& modes = VidModesToken;
    auto& rates = VidRefreshRateToken;

    if (!modes.empty())
        FreeVidModesToken();


    if (!rates.empty())
        FreeRefreshRateToken();

    int modeCount = SDL_GetNumDisplayModes(monitorID);
    R_ASSERT3(modeCount > 0, "Failed to find display modes", SDL_GetError());
    modes.reserve(modeCount + 1);

    SDL_DisplayMode displayMode;
    for (int i = modeCount - 1; i >= 0; --i)
    {
        R_ASSERT3(SDL_GetDisplayMode(monitorID, i, &displayMode) == 0, "Failed to find specified display mode", SDL_GetError());
        
        string16 str;
        xr_sprintf(str, sizeof(str), "%dx%d", displayMode.w, displayMode.h);

        if (modes.cend() == std::find_if(modes.cbegin(), modes.cend(), uniqueRenderingMode(str)))
            modes.emplace_back(xr_strdup(str), i);

        // For the first time we can fill refresh rate token here
        if (displayMode.w == psCurrentVidMode[0] && displayMode.h == psCurrentVidMode[1])
        {
            xr_itoa(displayMode.refresh_rate, str, 10);

            rates.emplace_back(xr_strdup(str), displayMode.refresh_rate);
        }
    }

    rates.emplace_back(nullptr, -1);

    Msg("Available video modes[%d]:", modes.size());
    for (const auto& mode : modes)
        Msg("[%s]", mode.name);
}

void FillRefreshRateToken()
{
    auto& rates = VidRefreshRateToken;

    if (!rates.empty())
        FreeRefreshRateToken();

    int modeCount = SDL_GetNumDisplayModes(Vid_SelectedMonitor);
    R_ASSERT3(modeCount > 0, "Failed to find display modes", SDL_GetError());

    SDL_DisplayMode displayMode;
    for (int i = modeCount - 1; i >= 0; --i)
    {
        R_ASSERT3(SDL_GetDisplayMode(Vid_SelectedMonitor, i, &displayMode) == 0, "Failed to find specified display mode",
            SDL_GetError());

        if (displayMode.w != psCurrentVidMode[0] || displayMode.h != psCurrentVidMode[1])
            continue;

        string16 buff;
        xr_itoa(displayMode.refresh_rate, buff, 10);

        rates.emplace_back(xr_strdup(buff), displayMode.refresh_rate);
    }

    rates.emplace_back(nullptr, -1);
}

void FreeMonitorsToken()
{
    for (auto& monitor : VidMonitorsToken)
        xr_free(monitor.name);

    VidMonitorsToken.clear();
}

void FreeVidModesToken()
{
    for (auto& mode : VidModesToken)
        xr_free(mode.name);

    VidModesToken.clear();
}

void FreeRefreshRateToken()
{
    for (auto& rate : VidRefreshRateToken)
        xr_free(rate.name);

    VidRefreshRateToken.clear();
}
