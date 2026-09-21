#include "stdafx.h"
#include "LightingMode.h"

namespace xray::render::fg
{
void LightingFrameState::Begin(bool requestRTGI, bool requestPT)
{
    requested = requestPT ? LightingMode::ReferencePT : (requestRTGI ? LightingMode::RTGI : LightingMode::Raster);
    effective = requested;
    fallback = LightingFallback::None;
    conflictingRequests = requestRTGI && requestPT;
    recorded = false;
    recordedSamples = 0;
    reuseRequested = false;
    reuseAvailable = false;
    reuseReservoirs = false;
    previousSurfacesValid = false;
    historyUsed = false;
    rtgiBounces = 0;
    rtgiSamples = 0;
    rtgiRayDistance = 0.0f;
    raySceneRadius = 0.0f;
    rayGrassRadius = 0.0f;
    rayGrassEnabled = false;
    rayGrassPending = false;
    rawSignalsRecorded = false;
    sceneRevision = 0;
}

void LightingFrameState::Fail(LightingFallback reason)
{
    effective = LightingMode::Raster;
    fallback = reason;
    recorded = false;
    recordedSamples = 0;
    reuseReservoirs = false;
    historyUsed = false;
    rawSignalsRecorded = false;
}

const char* LightingModeName(LightingMode mode)
{
    switch (mode)
    {
    case LightingMode::Raster:      return "Raster";
    case LightingMode::RTGI:        return "RTGI";
    case LightingMode::ReferencePT: return "Reference Path Tracer";
    }
    return "Unknown";
}

const char* RTGIImplementationName()
{
    return "raw multibounce v1";
}

const char* LightingFallbackName(LightingFallback reason)
{
    switch (reason)
    {
    case LightingFallback::None:                   return "ready";
    case LightingFallback::NoScene:                return "no active scene";
    case LightingFallback::Unsupported:            return "ray tracing unsupported";
    case LightingFallback::ShaderUnavailable:      return "lighting shader or reflection unavailable";
    case LightingFallback::PipelineUnavailable:    return "lighting pipeline unavailable";
    case LightingFallback::ResourcesUnavailable:   return "lighting resources unavailable";
    case LightingFallback::SceneUnavailable:       return "RT scene or build resources unavailable";
    case LightingFallback::EnvironmentUnavailable: return "sky textures unavailable";
    case LightingFallback::InputsUnavailable:      return "lighting frame inputs unavailable";
    case LightingFallback::BindingUnavailable:     return "lighting bindings unavailable";
    }
    return "unknown fallback";
}
}
