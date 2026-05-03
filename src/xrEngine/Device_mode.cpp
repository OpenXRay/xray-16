#include "stdafx.h"

#include "xrCore/xr_token.h"
#include "xr_input.h"

xr_vector<xr_token> vid_monitor_token;
xr_map<u32, xr_vector<xr_token>> vid_mode_token;
xr_vector<SDL_DisplayID> g_displayIDs;

SDL_DisplayID DisplayIDFromIndex(u32 index)
{
    if (index < g_displayIDs.size())
        return g_displayIDs[index];
    if (!g_displayIDs.empty())
        return g_displayIDs.front();
    return SDL_GetPrimaryDisplay();
}

void FillResolutionsForMonitor(int index, SDL_DisplayID displayID)
{
    int modeCount = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayID, &modeCount);
    if (!modes)
    {
        vid_mode_token[index].emplace_back(nullptr, -1);
        return;
    }

    xr_vector<std::pair<u32, u32>> seen;
    seen.reserve(modeCount);

    for (int i = 0; i < modeCount; ++i)
    {
        SDL_DisplayMode* mode = modes[i];
        if (!mode)
            continue;

        const auto wh = std::make_pair(static_cast<u32>(mode->w), static_cast<u32>(mode->h));
        if (std::find(seen.begin(), seen.end(), wh) != seen.end())
            continue;
        seen.push_back(wh);

        string64 buf;
        xr_sprintf(buf, sizeof(buf), "%dx%d", mode->w, mode->h);
        vid_mode_token[index].emplace_back(xr_strdup(buf), i);
    }

    SDL_free(modes);
    vid_mode_token[index].emplace_back(nullptr, -1);
}

void FillImGuiMonitorData(int index, SDL_DisplayID displayID)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGuiPlatformMonitor monitor;
    SDL_Rect r;
    SDL_GetDisplayBounds(displayID, &r);
    monitor.MainPos = monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
    monitor.MainSize = monitor.WorkSize = ImVec2((float)r.w, (float)r.h);

    SDL_GetDisplayUsableBounds(displayID, &r);
    monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
    monitor.WorkSize = ImVec2((float)r.w, (float)r.h);

    monitor.DpiScale = SDL_GetDisplayContentScale(displayID);
    if (monitor.DpiScale <= 0.0f)
        monitor.DpiScale = 1.0f;

    monitor.PlatformHandle = (void*)(intptr_t)index;
    platform_io.Monitors.push_back(monitor);
}

void CRenderDevice::FillVideoModes()
{
    ZoneScoped;

    g_displayIDs.clear();

    int displayCount = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    if (!displays || displayCount <= 0)
        return;

    for (int i = 0; i < displayCount; ++i)
    {
        const SDL_DisplayID id = displays[i];
        g_displayIDs.push_back(id);

        const char* name = SDL_GetDisplayName(id);
        string256 buf;
        xr_sprintf(buf, "%d. %s", i, name ? name : "(unknown)");
        vid_monitor_token.emplace_back(xr_strdup(buf), i);

        FillResolutionsForMonitor(i, id);
        FillImGuiMonitorData(i, id);
    }
    SDL_free(displays);
    vid_monitor_token.emplace_back(nullptr, -1);
}

void CRenderDevice::CleanupVideoModes()
{
    ZoneScoped;

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

    ImGui::GetPlatformIO().Monitors.resize(0);
}

void CRenderDevice::SetWindowDraggable(bool draggable)
{
    const bool windowed = psDeviceMode.WindowStyle == rsWindowed;
    const bool resizable = (SDL_GetWindowFlags(Device.m_sdlWnd) & SDL_WINDOW_RESIZABLE) != 0;
    m_allowWindowDrag = draggable && windowed && resizable;

    SDL_SetWindowOpacity(Device.m_sdlWnd, m_allowWindowDrag ? 0.95f : 1.0f);
}

void CRenderDevice::UpdateWindowProps()
{
    ZoneScoped;

    const bool windowed = psDeviceMode.WindowStyle != rsFullscreen;
    SelectResolution(windowed);

    if (SDL_GetDisplayForWindow(m_sdlWnd) != DisplayIDFromIndex(psDeviceMode.Monitor))
    {
        SDL_SetWindowFullscreen(m_sdlWnd, false);

        SDL_Rect rect;
        SDL_GetDisplayBounds(DisplayIDFromIndex(psDeviceMode.Monitor), &rect);
        SDL_SetWindowPosition(m_sdlWnd, rect.x, rect.y);
    }

    if (psDeviceMode.WindowStyle != rsFullscreenBorderless)
        SDL_SetWindowSize(m_sdlWnd, psDeviceMode.Width, psDeviceMode.Height);
    else
    {
        const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(DisplayIDFromIndex(psDeviceMode.Monitor));
        if (current)
            SDL_SetWindowSize(m_sdlWnd, current->w, current->h);
    }

    if (windowed)
    {
        const bool drawBorders = psDeviceMode.WindowStyle == rsWindowed;
        const bool useDesktopFullscreen = b_is_Ready && psDeviceMode.WindowStyle == rsFullscreenBorderless;

        SDL_SetWindowBordered(m_sdlWnd, drawBorders);
        SDL_SetWindowResizable(m_sdlWnd, !useDesktopFullscreen);
        if (useDesktopFullscreen)
        {
            SDL_SetWindowFullscreenMode(m_sdlWnd, nullptr);
            SDL_SetWindowFullscreen(m_sdlWnd, true);
        }
        else
        {
            SDL_SetWindowFullscreen(m_sdlWnd, false);
        }
    }
    else if (b_is_Ready)
    {
        SDL_SetWindowResizable(m_sdlWnd, false);

        SDL_DisplayMode mode{};
        mode.displayID = DisplayIDFromIndex(psDeviceMode.Monitor);
        mode.format = SDL_PIXELFORMAT_UNKNOWN;
        mode.w = static_cast<int>(psDeviceMode.Width);
        mode.h = static_cast<int>(psDeviceMode.Height);
        mode.refresh_rate = static_cast<float>(psDeviceMode.RefreshRate);
        SDL_SetWindowFullscreenMode(m_sdlWnd, &mode);
        SDL_SetWindowFullscreen(m_sdlWnd, true);
    }

    SDL_PumpEvents();
    UpdateWindowRects();

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = { static_cast<float>(psDeviceMode.Width), static_cast<float>(psDeviceMode.Height) };
    io.DisplayFramebufferScale = ImVec2{ float(dwWidth / m_rcWindowClient.w), float(dwHeight / m_rcWindowClient.h) };
}

void CRenderDevice::UpdateWindowRects()
{
    m_rcWindowClient.x = 0;
    m_rcWindowClient.y = 0;
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowClient.w, &m_rcWindowClient.h);

    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowBounds.x, &m_rcWindowBounds.y);
    SDL_GetWindowSize(m_sdlWnd, &m_rcWindowBounds.w, &m_rcWindowBounds.h);

    int top, left, bottom, right;
    SDL_GetWindowBordersSize(m_sdlWnd, &top, &left, &bottom, &right);
    m_rcWindowBounds.x -= left;
    m_rcWindowBounds.y -= top;
    m_rcWindowBounds.w += right;
    m_rcWindowBounds.h += bottom;
}

void CRenderDevice::SelectResolution(const bool windowed)
{
    if (GEnv.isDedicatedServer)
    {
        psDeviceMode.Width = 640;
        psDeviceMode.Height = 480;
    }
    else if (psDeviceMode.Width == 0 && psDeviceMode.Height == 0 && psDeviceMode.RefreshRate == 0)
    {
        const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(DisplayIDFromIndex(psDeviceMode.Monitor));
        if (current)
        {
            psDeviceMode.Width = current->w;
            psDeviceMode.Height = current->h;
            psDeviceMode.RefreshRate = static_cast<u32>(current->refresh_rate);
        }
    }
    else if (!windowed)
    {
        SDL_DisplayMode closest{};
        const bool ok = SDL_GetClosestFullscreenDisplayMode(
            DisplayIDFromIndex(psDeviceMode.Monitor),
            static_cast<int>(psDeviceMode.Width),
            static_cast<int>(psDeviceMode.Height),
            static_cast<float>(psDeviceMode.RefreshRate),
            false,
            &closest);

        if (!ok)
        {
            const SDL_DisplayMode* fallback = SDL_GetCurrentDisplayMode(DisplayIDFromIndex(psDeviceMode.Monitor));
            if (fallback)
                closest = *fallback;
        }

        psDeviceMode.Width = closest.w;
        psDeviceMode.Height = closest.h;
        psDeviceMode.RefreshRate = static_cast<u32>(closest.refresh_rate);
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
        SDL_SetWindowFullscreen(m_sdlWnd, false);

    if (needUpdateInput)
        pInput->GrabInput(restore);
}

void CRenderDevice::OnFatalError()
{
    SDL_SetWindowFullscreen(m_sdlWnd, false);
    SDL_SetWindowAlwaysOnTop(m_sdlWnd, false);
    SDL_ShowWindow(m_sdlWnd);
    SDL_MinimizeWindow(m_sdlWnd);
    SDL_HideWindow(m_sdlWnd);
}
