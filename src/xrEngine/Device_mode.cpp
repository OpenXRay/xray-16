#include "stdafx.h"

#include "xrCore/xr_token.h"
#include "xr_input.h"

xr_vector<xr_token> vid_monitor_token;
xr_map<u32, xr_vector<xr_token>> vid_mode_token;
xr_vector<SDL_DisplayID> g_displayIDs;

static SDL_DisplayID DisplayIDFromIndex(u32 index)
{
    if (index < g_displayIDs.size())
        return g_displayIDs[index];
    return SDL_GetPrimaryDisplay();
}

static void EnumerateMonitorModes(int index, SDL_DisplayID displayID)
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

static void RegisterImGuiMonitor(int index, SDL_DisplayID displayID)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGuiPlatformMonitor monitor;
    SDL_Rect r{};
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

        EnumerateMonitorModes(i, id);
        RegisterImGuiMonitor(i, id);
    }
    SDL_free(displays);
    vid_monitor_token.emplace_back(nullptr, -1);
}

void CRenderDevice::CleanupVideoModes()
{
    ZoneScoped;

    for (auto& [_, tokens] : vid_mode_token)
    {
        for (auto& token : tokens)
        {
            pstr name = const_cast<pstr>(token.name);
            xr_free(name);
        }
        tokens.clear();
    }
    vid_mode_token.clear();

    for (auto& token : vid_monitor_token)
    {
        pstr name = const_cast<pstr>(token.name);
        xr_free(name);
    }
    vid_monitor_token.clear();

    g_displayIDs.clear();

    ImGui::GetPlatformIO().Monitors.resize(0);
}

namespace
{
void SyncWindowedSize(SDL_Window* w)
{
    if (psDeviceMode.WindowStyle != rsWindowed)
        return;
    if (psDeviceMode.Width == 0 || psDeviceMode.Height == 0)
    {
        const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(DisplayIDFromIndex(psDeviceMode.Monitor));
        if (current)
        {
            psDeviceMode.Width = static_cast<u32>(current->w * 3 / 4);
            psDeviceMode.Height = static_cast<u32>(current->h * 3 / 4);
        }
    }
    int curW = 0, curH = 0;
    SDL_GetWindowSize(w, &curW, &curH);
    if (curW != static_cast<int>(psDeviceMode.Width) || curH != static_cast<int>(psDeviceMode.Height))
        SDL_SetWindowSize(w, static_cast<int>(psDeviceMode.Width), static_cast<int>(psDeviceMode.Height));
}

void SyncMonitor(SDL_Window* w)
{
    const SDL_DisplayID requested = DisplayIDFromIndex(psDeviceMode.Monitor);
    if (SDL_GetDisplayForWindow(w) == requested)
        return;
    SDL_Rect rect{};
    if (SDL_GetDisplayBounds(requested, &rect))
        SDL_SetWindowPosition(w, rect.x, rect.y);
}

void SyncFullscreenMode(SDL_Window* w)
{
    if (psDeviceMode.WindowStyle != rsFullscreen)
    {
        SDL_SetWindowFullscreenMode(w, nullptr);
        return;
    }
    SDL_DisplayMode closest{};
    if (SDL_GetClosestFullscreenDisplayMode(
            DisplayIDFromIndex(psDeviceMode.Monitor),
            static_cast<int>(psDeviceMode.Width),
            static_cast<int>(psDeviceMode.Height),
            static_cast<float>(psDeviceMode.RefreshRate),
            false,
            &closest))
    {
        psDeviceMode.Width = static_cast<u32>(closest.w);
        psDeviceMode.Height = static_cast<u32>(closest.h);
        psDeviceMode.RefreshRate = static_cast<u32>(closest.refresh_rate);
        SDL_SetWindowFullscreenMode(w, &closest);
    }
    else
    {
        SDL_SetWindowFullscreenMode(w, nullptr);
    }
}

void SyncWindowMode(SDL_Window* w)
{
    const u32 mode = psDeviceMode.WindowStyle;
    SDL_SetWindowBordered(w, mode == rsWindowed);
    SDL_SetWindowResizable(w, mode == rsWindowed);
    SDL_SetWindowFullscreen(w, mode != rsWindowed);
}
}

void CRenderDevice::SyncWindowToPsDeviceMode()
{
    ZoneScoped;
    if (!m_sdlWnd)
        return;

    SyncMonitor(m_sdlWnd);
    SyncWindowedSize(m_sdlWnd);
    SyncFullscreenMode(m_sdlWnd);
    SyncWindowMode(m_sdlWnd);
}

void CRenderDevice::UpdateWindowState()
{
    ZoneScoped;
    if (!m_sdlWnd)
        return;

    int pxW = 0, pxH = 0;
    SDL_GetWindowSizeInPixels(m_sdlWnd, &pxW, &pxH);
    m_windowVisible = (pxW > 0 && pxH > 0);
    if (!m_windowVisible)
        return;

    if (static_cast<u32>(pxW) != dwWidth || static_cast<u32>(pxH) != dwHeight)
    {
        if (psDeviceMode.WindowStyle == rsWindowed)
        {
            psDeviceMode.Width = static_cast<u32>(pxW);
            psDeviceMode.Height = static_cast<u32>(pxH);
        }
        Reset();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { static_cast<float>(dwWidth), static_cast<float>(dwHeight) };
    io.DisplayFramebufferScale = ImVec2{ 1.0f, 1.0f };
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
        SyncWindowToPsDeviceMode();
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
