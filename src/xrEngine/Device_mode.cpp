#include "stdafx.h"

#include "xrCore/xr_token.h"
#include "xr_input.h"

xr_vector<xr_token> vid_monitor_token;
xr_map<u32, xr_vector<xr_token>> vid_mode_token;

void FillResolutionsForMonitor(const int monitorID)
{
    const int modeCount = SDL_GetNumDisplayModes(monitorID);
    R_ASSERT3(modeCount > 0, "Failed to find display modes", SDL_GetError());

    for (int i = modeCount - 1; i >= 0; --i)
    {
        SDL_DisplayMode mode;
        const int result = SDL_GetDisplayMode(monitorID, i, &mode);
        R_ASSERT3(result == 0, "Failed to find specified display mode", SDL_GetError());

        string256 buf;
        xr_sprintf(buf, sizeof(buf), "%ux%u (%dHz)", mode.w, mode.h, mode.refresh_rate);
        vid_mode_token[monitorID].emplace_back(xr_strdup(buf), i);
    }

    vid_mode_token[monitorID].emplace_back(nullptr, -1);
}

void CRenderDevice::FillVideoModes()
{
    const int displayCount = SDL_GetNumVideoDisplays();
    R_ASSERT3(displayCount > 0, "Failed to find display", SDL_GetError());

    for (int i = 0; i < displayCount; ++i)
    {
        string256 buf;
        xr_sprintf(buf, "%d. %s", i, SDL_GetDisplayName(i));
        vid_monitor_token.emplace_back(xr_strdup(buf), i);

        FillResolutionsForMonitor(i);
    }
    vid_monitor_token.emplace_back(nullptr, -1);
}

void CRenderDevice::CleanupVideoModes()
{
    for (auto& [monitor_id, tokens] : vid_mode_token)
    {
        for (auto& token : tokens)
        {
            auto tokenName = const_cast<pstr>(token.name);
            xr_free(tokenName);
        }
        tokens.clear();
    }
    vid_mode_token.clear();

    for (auto& token : vid_monitor_token)
    {
        pstr tokenName = const_cast<pstr>(token.name);
        xr_free(tokenName);
    }
    vid_monitor_token.clear();
}

void CRenderDevice::SetWindowDraggable(bool draggable)
{
    // Only draggable if resizable too
    const bool windowed = psDeviceMode.WindowStyle == rsWindowed || psDeviceMode.WindowStyle == rsWindowedBorderless;
    const bool resizable = SDL_GetWindowFlags(Device.m_sdlWnd) & SDL_WINDOW_RESIZABLE;
    m_allowWindowDrag = draggable && windowed && resizable;

#if SDL_VERSION_ATLEAST(2, 0, 5)
    SDL_SetWindowOpacity(Device.m_sdlWnd, m_allowWindowDrag ? 0.95f : 1.0f);
#endif
}

void CRenderDevice::UpdateWindowProps()
{
    const bool windowed = psDeviceMode.WindowStyle != rsFullscreen;
    SelectResolution(windowed);

    // Changing monitor, unset fullscreen for the previous monitor
    // and move the window to the new monitor
    if (SDL_GetWindowDisplayIndex(m_sdlWnd) != static_cast<int>(psDeviceMode.Monitor))
    {
        SDL_SetWindowFullscreen(m_sdlWnd, SDL_DISABLE);

        SDL_Rect rect;
        SDL_GetDisplayBounds(psDeviceMode.Monitor, &rect);
        SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);
    }

    SDL_SetWindowSize(m_sdlWnd, psDeviceMode.Width, psDeviceMode.Height);

    if (windowed)
    {
        const bool drawBorders = psDeviceMode.WindowStyle == rsWindowed;
        const bool useDesktopFullscreen = b_is_Ready && psDeviceMode.WindowStyle == rsFullscreenBorderless;

        SDL_SetWindowBordered(m_sdlWnd, drawBorders ? SDL_TRUE : SDL_FALSE);
        SDL_SetWindowResizable(m_sdlWnd, !useDesktopFullscreen ? SDL_TRUE : SDL_FALSE);
        SDL_SetWindowFullscreen(m_sdlWnd, useDesktopFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_DISABLE);
    }
    else if (b_is_Ready)
    {
        SDL_SetWindowResizable(m_sdlWnd, SDL_FALSE);
        SDL_SetWindowFullscreen(m_sdlWnd, SDL_WINDOW_FULLSCREEN);

        SDL_DisplayMode mode;
        SDL_GetWindowDisplayMode(m_sdlWnd, &mode);
        mode.w = psDeviceMode.Width;
        mode.h = psDeviceMode.Height;
        mode.refresh_rate = psDeviceMode.RefreshRate;
        SDL_SetWindowDisplayMode(m_sdlWnd, &mode);
    }

    UpdateWindowRects();
    SDL_FlushEvents(SDL_WINDOWEVENT, SDL_SYSWMEVENT);

    editor().UpdateWindowProps();
}

void CRenderDevice::UpdateWindowRects()
{
    m_rcWindowClient.x = 0;
    m_rcWindowClient.y = 0;
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowClient.w, &m_rcWindowClient.h);

    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowBounds.x, &m_rcWindowBounds.y);
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowBounds.w, &m_rcWindowBounds.h);

#if SDL_VERSION_ATLEAST(2, 0, 5)
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(m_sdlWnd, &top, &left, &bottom, &right);
    m_rcWindowBounds.x -= left;
    m_rcWindowBounds.y -= top;
    m_rcWindowBounds.w += right;
    m_rcWindowBounds.h += bottom;
#endif
}

void CRenderDevice::SelectResolution(const bool windowed)
{
    // Dedicated server hardcoded resolution
    // XXX: to be removed
    if (GEnv.isDedicatedServer)
    {
        psDeviceMode.Width = 640;
        psDeviceMode.Height = 480;
    }
    else if (psDeviceMode.Width == 0 && psDeviceMode.Height == 0 && psDeviceMode.RefreshRate == 0)
    {
        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(psDeviceMode.Monitor, &current);
        psDeviceMode.Width = current.w;
        psDeviceMode.Height = current.h;
        psDeviceMode.RefreshRate = current.refresh_rate;
    }
    else if (!windowed) // check if safe for fullscreen
    {
        string256 buf;
        xr_sprintf(buf, "%ux%u (%dHz)", psDeviceMode.Width, psDeviceMode.Height, psDeviceMode.RefreshRate);

        auto modes = vid_mode_token[psDeviceMode.Monitor];
        const auto it = std::find_if(modes.begin(), modes.end(), [&buf](const xr_token& token)
        {
            return token.name && xr_strcmp(token.name, buf) == 0;
        });

        if (it == modes.end()) // not found
        {
            SDL_DisplayMode current =
            {
                SDL_PIXELFORMAT_UNKNOWN,
                (int)psDeviceMode.Width,
                (int)psDeviceMode.Height,
                (int)psDeviceMode.RefreshRate,
                nullptr
            };

            SDL_DisplayMode closest; // try closest or fallback to desktop mode
            if (!SDL_GetClosestDisplayMode(psDeviceMode.Monitor, &current, &closest))
            {
                SDL_GetCurrentDisplayMode(psDeviceMode.Monitor, &closest);
            }

            psDeviceMode.Width = closest.w;
            psDeviceMode.Height = closest.h;
            psDeviceMode.RefreshRate = closest.refresh_rate;
        }
    }

    dwWidth = psDeviceMode.Width;
    dwHeight = psDeviceMode.Height;
}

SDL_Window* CRenderDevice::GetApplicationWindow()
{
    return m_sdlWnd;
}

void CRenderDevice::OnErrorDialog(bool beforeDialog)
{
    const bool restore = !beforeDialog;
    const bool needUpdateInput = pInput && pInput->IsExclusiveMode();

    if (restore)
        UpdateWindowProps();
    else
        SDL_SetWindowFullscreen(m_sdlWnd, SDL_FALSE);

    if (needUpdateInput)
        pInput->GrabInput(restore);
}

void CRenderDevice::OnFatalError()
{
    // make it sure window will hide in any way
    SDL_SetWindowFullscreen(m_sdlWnd, SDL_FALSE);
#if SDL_VERSION_ATLEAST(2, 0, 16)
    SDL_SetWindowAlwaysOnTop(m_sdlWnd, SDL_FALSE);
#endif
    SDL_ShowWindow(m_sdlWnd);
    SDL_MinimizeWindow(m_sdlWnd);
    SDL_HideWindow(m_sdlWnd);
}
