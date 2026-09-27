#pragma once

#include "xrCore/xr_types.h"

namespace xray::render::fg
{
enum class LightingMode : u8
{
    Raster,
    RTGI,
    RadianceCascades,
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
    BindingUnavailable,
    RecordingUnavailable
};

class LightingFrameState
{
public:
    void Begin(LightingMode request);
    void ScheduleOpaqueLighting();
    void Fail(LightingFallback reason);
    void ResetRecovery();

    LightingMode requested = LightingMode::Raster;
    LightingMode effective = LightingMode::Raster;
    LightingMode scheduled = LightingMode::Raster;
    LightingFallback fallback = LightingFallback::None;
    u32 recordedSamples = 0;
    bool rayTracing = false;
    bool opaqueScheduled = false;
    bool frameFailed = false;
    bool recoveryActive = false;
    bool failureCleared = false;
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
    u32 rayStaticDetailInstances = 0;
    bool rawSignalsRecorded = false;
    bool reconstructionRequested = false;
    bool reconstructionActive = false;
    LightingFallback reconstructionFallback = LightingFallback::None;
    u32 reconstructionHistory = 0;
    bool reconstructionSpatialRequested = false;
    bool reconstructionSpatialActive = false;
    LightingFallback reconstructionSpatialFallback = LightingFallback::None;
    u32 reconstructionFilterPasses = 0;
    u64 sceneRevision = 0;
    u64 poseRevision = 0;
    bool worldCacheRequested = false;
    bool worldCacheScheduled = false;
    bool worldCacheSelectRecorded = false;
    bool worldCacheUpdateRecorded = false;
    bool worldCacheRecorded = false;
    bool worldCacheLiveCellsKnown = false;
    bool worldCacheEventsKnown = false;
    LightingFallback worldCacheFallback = LightingFallback::None;
    u32 worldCacheBounce = 0;
    u32 worldCacheMaxBounces = 0;
    u32 worldCacheUpdates = 0;
    u32 worldCacheDebug = 0;
    u32 worldCacheLifetime = 0;
    u32 worldCacheCapacity = 0;
    u32 worldCacheLiveCells = 0;
    u32 worldCacheUpdated = 0;
    u32 worldCacheSubstituted = 0;
    u32 worldCacheUnsampled = 0;
    u32 worldCacheAbsent = 0;
    u32 worldCacheBypassed = 0;
    float worldCacheCellSize = 0.0f;
    u64 worldCacheBytes = 0;

private:
    LightingMode m_latchedMode = LightingMode::Raster;
    LightingFallback m_latchedReason = LightingFallback::None;
};

LightingMode LightingModeFromSetting(int setting);
bool LightingModeUsesRays(LightingMode mode);
const char* LightingModeName(LightingMode mode);
const char* LightingFallbackName(LightingFallback reason);
const char* RTGIImplementationName();
}
