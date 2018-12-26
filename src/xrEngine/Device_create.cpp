#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "xrCore/xr_token.h"
#include "xrCDB/xrXRC.h"
#include "XR_IOConsole.h"
#include "SDL.h"	
#include "SDL_syswm.h"

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
        u32 width, height;
        sscanf(VidModesToken[VidModesToken.size() - 2].name, "%dx%d", &width, &height);

        const bool drawBorders = strstr(Core.Params, "-draw_borders");

        bool maximalResolution = false;
        if (b_is_Ready && !drawBorders && psCurrentVidMode[0] == width && psCurrentVidMode[1] == height)
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
                xr_sprintf(buff, sizeof(buff), "vid_mode %s", VidModesToken.back());

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
    {
        string512 buf;
        xr_sprintf(buf, sizeof(buf), "%d. %s", i + 1, SDL_GetDisplayName(i));
        monitors.emplace_back(xr_strdup(buf), i);
    }

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

    Msg("Available video modes[%d]:", modes.size());

    SDL_DisplayMode displayMode;
    for (int i = modeCount - 1; i >= 0; --i)
    {
        R_ASSERT3(SDL_GetDisplayMode(monitorID, i, &displayMode) == 0, "Failed to find specified display mode", SDL_GetError());
        
        string16 str;
        xr_sprintf(str, sizeof(str), "%dx%d", displayMode.w, displayMode.h);

        if (modes.cend() == std::find_if(modes.cbegin(), modes.cend(), uniqueRenderingMode(str)))
        {
            modes.emplace_back(xr_strdup(str), i);
            Msg("[%s]", str);
        }

        // For the first time we can fill refresh rate token here
        if (displayMode.w == psCurrentVidMode[0] && displayMode.h == psCurrentVidMode[1])
        {
            xr_itoa(displayMode.refresh_rate, str, 10);

            rates.emplace_back(xr_strdup(str), displayMode.refresh_rate);
        }
    }

    modes.emplace_back(nullptr, -1);
    rates.emplace_back(nullptr, -1);
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
