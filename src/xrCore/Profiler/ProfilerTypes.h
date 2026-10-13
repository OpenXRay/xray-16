#pragma once

#include "xrCore/xr_types.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"
#include "xrCore/xrstring.h"

#include <atomic>

namespace xray::profiler
{

constexpr u32 INVALID_ZONE_ID = ~0u;

struct ZoneInfo
{
    const char* name;
    const char* file;
    u32 line;
    mutable std::atomic<u32> id{INVALID_ZONE_ID};
};

struct ZoneTiming
{
    float totalTimeMs = 0.0f;
    float selfTimeMs = 0.0f;
    u32 callCount = 0;
    u32 parentId = INVALID_ZONE_ID;
    xr_vector<u32> childIds;

    u64 allocCalls = 0;
    u64 allocBytes = 0;
    u64 freeCalls = 0;
    u64 freeBytes = 0;
    u64 selfAllocCalls = 0;
    u64 selfAllocBytes = 0;

    void Reset()
    {
        totalTimeMs = 0.0f;
        selfTimeMs = 0.0f;
        callCount = 0;
        allocCalls = 0;
        allocBytes = 0;
        freeCalls = 0;
        freeBytes = 0;
        selfAllocCalls = 0;
        selfAllocBytes = 0;
    }
};

struct ZoneData
{
    const ZoneInfo* info = nullptr;
    u32 zoneId = INVALID_ZONE_ID;
    ZoneTiming timing;
    double allocAverageCalls = 0.0;
    u32 allocSampleCount = 0;

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
