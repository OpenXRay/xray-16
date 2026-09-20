// Stats.h: interface for the CStats class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_flags.h"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"
#include <array>

class ENGINE_API CGameFont;

DECLARE_MESSAGE(Stats);

class ENGINE_API CStats : public pureRender
{
private:
    CGameFont* statsFont;
    struct FPSSample
    {
        double time = 0.0;
        float fps = 0.f;
    };
    static constexpr u32 fpsHistorySize = 64;
    std::array<FPSSample, fpsHistorySize> fpsHistory{};
    u32 fpsHistoryWrite = 0;
    u32 fpsHistoryCount = 0;
    u64 fpsFrameCount = 0;
    double fpsElapsed = 0.0;
    double fpsSampleTime = 0.0;
    u32 fpsSampleFrames = 0;
    float fpsAverage = 0.f;
    float fpsMinimum = 0.f;
    float fpsMaximum = 0.f;
    xr_vector<shared_str> errors;

public:
    CStats();
    ~CStats();

    void Show(void);
    void RenderFPSOverlay();
    virtual void OnRender();
    void OnDeviceCreate(void);
    void OnDeviceDestroy(void);

private:
    void ResetFPSOverlay();
    void FilteredLog(const char* s);
};

enum
{
    st_sound = (1 << 0),
    st_sound_min_dist = (1 << 1),
    st_sound_max_dist = (1 << 2),
    st_sound_ai_dist = (1 << 3),
    st_sound_info_name = (1 << 4),
    st_sound_info_object = (1 << 5),
};

extern ENGINE_API CStatTimer gTestTimer0; // debug counter
extern ENGINE_API CStatTimer gTestTimer1; // debug counter
extern ENGINE_API CStatTimer gTestTimer2; // debug counter
extern ENGINE_API CStatTimer gTestTimer3; // debug counter

extern Flags32 g_stats_flags;
