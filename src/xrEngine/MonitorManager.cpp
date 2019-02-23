#include "stdafx.h"

#include "MonitorManager.hpp"

ENGINE_API u32 Vid_SelectedMonitor = 0;
ENGINE_API u32 Vid_SelectedRefreshRate = 60;

MonitorsManager g_monitors;

void MonitorsManager::Initialize()
{
    FillMonitorsMap();
}

void MonitorsManager::Destroy()
{
    Monitors.clear();
}

u32 MonitorsManager::GetMonitorsCount()
{
    return Monitors.size();
}

MonitorsManager::RefreshRatesVec* MonitorsManager::GetRefreshRates()
{
    const ResolutionPair selected = { psCurrentVidMode[0], psCurrentVidMode[1] };

    ResolutionsMap& resolutions = Monitors[Vid_SelectedMonitor];
    const auto it = resolutions.find(selected);

    if (it != resolutions.end())
        return &it->second;

    return nullptr;
}

MonitorsManager::ResolutionPair MonitorsManager::GetMinimalResolution()
{
    const ResolutionsMap& resolutions = Monitors[Vid_SelectedMonitor];
    const auto it = resolutions.cbegin();
    return it->first;
}

MonitorsManager::ResolutionPair MonitorsManager::GetMaximalResolution()
{
    const ResolutionsMap& resolutions = Monitors[Vid_SelectedMonitor];
    const auto it = resolutions.crbegin();
    return it->first;
}


const MonitorsManager::TokenVector& MonitorsManager::GetTokensForCurrentMonitor()
{
    for (auto& token : tokens)
        xr_free(token.name);
    tokens.clear();

    int i = 0;
    const auto pushString = [&](u32 w, u32 h)
    {
        string64 buf;
        xr_sprintf(buf, sizeof(buf), "%dx%d", w, h);
        tokens.emplace_back(xr_strdup(buf), i++); // It's important to have postfix increment!
    };

    for (const auto& map : Monitors[Vid_SelectedMonitor])
    {
        const ResolutionPair resolution = map.first;

        pushString(resolution.first, resolution.second);
    }

    tokens.emplace_back(nullptr, -1);

    return tokens;
}

bool MonitorsManager::SelectedResolutionIsSafe()
{
    const ResolutionsMap& resolutions = Monitors[Vid_SelectedMonitor];
    const ResolutionPair selected = { psCurrentVidMode[0], psCurrentVidMode[1] };
    const auto it = resolutions.find(selected);

    return it != resolutions.end();
}

void MonitorsManager::FillMonitorsTips(IConsole_Command::vecTips& tips)
{
    const auto pushString = [&](pcstr fmt, u32 id)
    {
        string64 buf;
        xr_sprintf(buf, sizeof(buf), fmt, id + 1, SDL_GetDisplayName(id));
        tips.push_back(buf);
    };

    pushString("%d. %s (current)", Vid_SelectedMonitor);

    auto it = Monitors.begin();
    const auto ite = Monitors.end();

    for (; it != ite; ++it)
    {
        pushString("%d. %s", it->first);
    }
}

void MonitorsManager::FillResolutionsTips(IConsole_Command::vecTips& tips)
{
    const auto pushString = [&](pcstr fmt, u32 w, u32 h)
    {
        string64 buf;
        xr_sprintf(buf, sizeof(buf), fmt, w, h);
        tips.push_back(buf);
    };

    pushString("%dx%d (current)", psCurrentVidMode[0], psCurrentVidMode[1]);

    for (const auto& map : Monitors[Vid_SelectedMonitor])
    {
        const ResolutionPair resolution = map.first;

        pushString("%dx%d", resolution.first, resolution.second);
    }
}

void MonitorsManager::FillRatesTips(IConsole_Command::vecTips& tips)
{
    const auto pushString = [&](pcstr fmt, u32 rate)
    {
        string16 buf;
        xr_sprintf(buf, sizeof(buf), fmt, rate);
        tips.push_back(buf);
    };

    pushString("%d (current)", Vid_SelectedRefreshRate);

    ResolutionsMap& monitor = Monitors[Vid_SelectedMonitor];
    const RefreshRatesVec& resolution = monitor[{psCurrentVidMode[0], psCurrentVidMode[1]}];

    for (const auto& rate : resolution)
    {
        pushString("%d", rate);
    }
}

void MonitorsManager::FillMonitorsMap()
{
    const int displayCount = SDL_GetNumVideoDisplays();
    R_ASSERT3(displayCount > 0, "Failed to find display", SDL_GetError());

    for (int i = 0; i < displayCount; ++i)
    {
        FillResolutionsModes(i, Monitors[i]);
    }
}

void MonitorsManager::FillResolutionsModes(const int monitorID, ResolutionsMap& map)
{
    const int modeCount = SDL_GetNumDisplayModes(monitorID);
    R_ASSERT3(modeCount > 0, "Failed to find display modes", SDL_GetError());

    for (int i = modeCount - 1; i >= 0; --i)
    {
        SDL_DisplayMode mode;
        const int result = SDL_GetDisplayMode(monitorID, i, &mode);
        R_ASSERT3(result == 0, "Failed to find specified display mode", SDL_GetError());

        map[{mode.w, mode.h}].push_back(mode.refresh_rate);
    }
}
