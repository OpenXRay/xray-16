#pragma once

#include "xrCore/xr_types.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"
#include "xrCore/xrstring.h"

namespace xray::profiler
{

constexpr u32 INVALID_ZONE_ID = ~0u;

struct ZoneInfo
{
    const char* name;
    const char* file;
    u32 line;
    mutable u32 id = INVALID_ZONE_ID;
};

struct ZoneTiming
{
    float totalTimeMs = 0.0f;
    float selfTimeMs = 0.0f;
    u32 callCount = 0;
    u32 parentId = INVALID_ZONE_ID;
    xr_vector<u32> childIds;

    void Reset()
    {
        totalTimeMs = 0.0f;
        selfTimeMs = 0.0f;
        callCount = 0;
    }
};

struct ZoneData
{
    const ZoneInfo* info = nullptr;
    ZoneTiming timing;

    u32 parentId = INVALID_ZONE_ID;
    xr_vector<u32> childIds;
};

struct GPUPassTiming
{
    shared_str name;
    float timeMs = 0.0f;
    bool pending = false;
    bool isAsync = false;
};

struct FrameStats
{
    float cpuFrameTimeMs = 0.0f;
    float gpuFrameTimeMs = 0.0f;
    u32 cpuZoneCount = 0;
    u32 gpuPassCount = 0;
};

}
