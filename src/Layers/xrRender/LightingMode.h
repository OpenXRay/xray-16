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
    bool reuseRequested = false;
    bool reuseAvailable = false;
    bool reuseReservoirs = false;
    bool previousSurfacesValid = false;
    bool historyUsed = false;
    u32 rtgiBounces = 0;
    u32 rtgiSamples = 0;
    float rtgiRayDistance = 0.0f;
    float raySceneRadius = 0.0f;
    float rayGrassRadius = 0.0f;
    bool rayGrassEnabled = false;
    bool rayGrassPending = false;
    bool rawSignalsRecorded = false;
    u64 sceneRevision = 0;
};

const char* LightingModeName(LightingMode mode);
const char* LightingFallbackName(LightingFallback reason);
const char* RTGIImplementationName();
}
