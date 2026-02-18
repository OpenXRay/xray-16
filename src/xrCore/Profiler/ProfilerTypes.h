#pragma once

#include "../xr_types.h"
#include "../xrCommon/xr_vector.h"
#include "../xrCommon/xr_map.h"
#include "../xrstring.h"

namespace xray::profiler
{

constexpr u32 INVALID_ZONE_ID = ~0u;

// Static zone metadata (one instance per source location)
struct ZoneInfo
{
    const char* name;
    const char* file;
    u32 line;
    mutable u32 id = INVALID_ZONE_ID;  // Assigned on first use
};

// Per-frame timing data for a single zone
struct ZoneTiming
{
    float totalTimeMs = 0.0f;    // Total time including children
    float selfTimeMs = 0.0f;     // Time excluding children (computed after frame)
    u32 callCount = 0;
    u32 parentId = INVALID_ZONE_ID;
    xr_vector<u32> childIds;

    void Reset()
    {
        totalTimeMs = 0.0f;
        selfTimeMs = 0.0f;
        callCount = 0;
        // Note: parentId and childIds are structural, reset separately
    }
};

// Complete zone data combining static info and timing
struct ZoneData
{
    const ZoneInfo* info = nullptr;
    ZoneTiming timing;

    // For tree structure
    u32 parentId = INVALID_ZONE_ID;
    xr_vector<u32> childIds;
};

// GPU pass timing (simpler flat structure for FrameGraph passes)
struct GPUPassTiming
{
    shared_str name;
    float timeMs = 0.0f;
    bool pending = false;  // Waiting for GPU results
    bool isAsync = false;  // Ran on async compute queue
};

// Frame statistics summary
struct FrameStats
{
    float cpuFrameTimeMs = 0.0f;
    float gpuFrameTimeMs = 0.0f;
    u32 cpuZoneCount = 0;
    u32 gpuPassCount = 0;
};

} // namespace xray::profiler
