#pragma once

#include "xrCore/xr_types.h"

namespace xray::render::fg
{
enum class LightingMode : u8
{
    Raster,
    RTGI,
    ReferencePT
};

enum class LightingFallback : u8
{
    None,
    NoScene,
    Unsupported,
    ShaderUnavailable,
    PipelineUnavailable,
    ResourcesUnavailable,
    SceneUnavailable,
    EnvironmentUnavailable,
    InputsUnavailable,
    BindingUnavailable
};

class LightingFrameState
{
public:
    void Begin(bool requestRTGI, bool requestPT);
    void Fail(LightingFallback reason);

    LightingMode requested = LightingMode::Raster;
    LightingMode effective = LightingMode::Raster;
    LightingFallback fallback = LightingFallback::None;
    u32 recordedSamples = 0;
    bool conflictingRequests = false;
    bool recorded = false;
    bool reuseReservoirs = true;
};

const char* LightingModeName(LightingMode mode);
const char* LightingFallbackName(LightingFallback reason);
}
