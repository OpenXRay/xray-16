#pragma once

#include "xrCore/xrCore.h"
#include <cstddef>

namespace xray::render::fg
{
class SkyProbeDebugRay
{
public:
    Fvector4 metrics = {};
    u32 hit[4] = {};
};
static_assert(sizeof(SkyProbeDebugRay) == 32);

class SkyProbeDebugCorner
{
public:
    u32 identity[4] = {};
    u32 cellAndFlags[4] = {};
    u32 rawVisibility[4] = {};
    u32 rawDepthMean[4] = {};
    u32 rawDepthSigma[4] = {};
    Fvector4 moment = {};
    Fvector4 position = {};
    Fvector4 offset = {};
    Fvector4 coefficients = {};
    Fvector4 weights = {};
    Fvector4 sky = {};
    Fvector4 contribution = {};
    SkyProbeDebugRay rays[6];
};
static_assert(sizeof(SkyProbeDebugCorner) == 384);
static_assert(offsetof(SkyProbeDebugCorner, rawVisibility) == 2 * 16);
static_assert(offsetof(SkyProbeDebugCorner, rawDepthMean) == 3 * 16);
static_assert(offsetof(SkyProbeDebugCorner, rawDepthSigma) == 4 * 16);
static_assert(offsetof(SkyProbeDebugCorner, moment) == 5 * 16);
static_assert(offsetof(SkyProbeDebugCorner, position) == 6 * 16);
static_assert(offsetof(SkyProbeDebugCorner, weights) == 9 * 16);
static_assert(offsetof(SkyProbeDebugCorner, contribution) == 11 * 16);
static_assert(offsetof(SkyProbeDebugCorner, rays) == 12 * 16);

class SkyProbeDebugSnapshot
{
public:
    static constexpr u32 kVersion = 2;
    static constexpr u32 kProductionMoments = 1;
    static constexpr u32 kBufferStride = 16;
    static constexpr u32 kBufferRows = 212;
    static constexpr u32 kHeaderRows = 20;
    static constexpr u32 kCornerRows = 24;
    static constexpr float kVisibilityFloor = 0.05f;
    u32 metadata[4] = {};
    u32 selection[4] = {};
    u32 gridDims[4] = {};
    Fvector4 gridOrigin = {};
    Fvector4 camera = {};
    Fvector4 surface = {};
    Fvector4 shadingNormal = {};
    Fvector4 geometricNormal = {};
    Fvector4 material = {};
    Fvector4 view = {};
    Fvector4 biased = {};
    Fvector4 rayOrigin = {};
    Fvector4 controlOrigin = {};
    Fvector4 sampleSH = {};
    Fvector4 summary = {};
    Fvector4 sky = {};
    Fvector4 reflection = {};
    Fvector4 settings = {};
    Fvector4 lighting = {};
    u32 scene[4] = {};
    SkyProbeDebugCorner corners[8];
};
static_assert(sizeof(SkyProbeDebugSnapshot) == 3392);
static_assert(sizeof(SkyProbeDebugSnapshot) == SkyProbeDebugSnapshot::kBufferStride * SkyProbeDebugSnapshot::kBufferRows);
static_assert(offsetof(SkyProbeDebugSnapshot, corners) == SkyProbeDebugSnapshot::kHeaderRows * SkyProbeDebugSnapshot::kBufferStride);
static_assert(sizeof(SkyProbeDebugCorner) == SkyProbeDebugSnapshot::kCornerRows * SkyProbeDebugSnapshot::kBufferStride);
static_assert(SkyProbeDebugSnapshot::kBufferRows ==
    SkyProbeDebugSnapshot::kHeaderRows + 8 * SkyProbeDebugSnapshot::kCornerRows);
}
