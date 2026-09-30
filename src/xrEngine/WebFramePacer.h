#pragma once

#ifdef XR_PLATFORM_WEB
#include <atomic>

class CWebFramePacer
{
public:
    void SetVsyncPaced(bool paced);
    void SetFrameLimit(double hz) { frameLimitHz = hz; }
    bool IsFrameDue(double now);

private:
    void RecordFrame(u32 refreshes, u32 maxInterval);
    void ChangeInterval(u32 value);

    static constexpr u32 MaxInterval = 4;
    static constexpr double MinPacedHz = 58.0;
    static constexpr u32 MissWindow = 60;
    static constexpr u32 MissLimit = 6;
    static constexpr u32 SpareWindow = 300;
    static constexpr u32 MaxSpareWindow = 9600;
    static constexpr double QuickFailMs = 20000.0;
    static constexpr double StallMs = 250.0;

    std::atomic<s32> displayPeriodUs{};
    bool displayHookInstalled{};
    bool vsyncPaced{};
    bool lastTickRendered{};
    bool lastChangeDown{};
    double lastTick{};
    double frameLimitHz{};
    u32 interval{ 1 };
    u32 refreshesSinceFrame{};
    u32 missFrames{};
    u32 misses{};
    u32 spareFrames{};
    u32 spareWindow{ SpareWindow };
    double changeTime{};
};

extern CWebFramePacer g_webFramePacer;
#endif
