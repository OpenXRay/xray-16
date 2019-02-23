#pragma once

#include "XR_IOConsole.h" // don't remove
#include "xr_ioc_cmd.h"

// Monitor -> Resolutions per monitor -> Refresh rates per resolution
class ENGINE_API MonitorsManager
{
public:
    using RefreshRatesVec = xr_vector<u32>;
    using ResolutionPair = std::pair<u32, u32>;

    using ResolutionsMap = xr_map<ResolutionPair, RefreshRatesVec>;

    using MonitorMap = xr_map<u32, ResolutionsMap>;

    using TokenVector = xr_vector<xr_token>;

private:
    MonitorMap Monitors;
    TokenVector tokens;

public:
    MonitorsManager() = default;
    ~MonitorsManager() = default;

    void Initialize();
    void Destroy();

    u32 GetMonitorsCount();

    // For current resolution
    RefreshRatesVec* GetRefreshRates(); 

    ResolutionPair GetMinimalResolution();
    ResolutionPair GetMaximalResolution();

    // Not thread-safe, for backwards compatibility only
    const TokenVector& GetTokensForCurrentMonitor();

    bool SelectedResolutionIsSafe();

    void FillMonitorsTips(IConsole_Command::vecTips& tips);
    void FillResolutionsTips(IConsole_Command::vecTips& tips);
    void FillRatesTips(IConsole_Command::vecTips& tips);

private:
    void FillMonitorsMap();
    void FillResolutionsModes(const int monitorID, ResolutionsMap& map);
};

extern ENGINE_API MonitorsManager g_monitors;
