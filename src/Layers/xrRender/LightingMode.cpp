#include "stdafx.h"
#include "LightingMode.h"

namespace xray::render::fg
{
void LightingFrameState::Begin(bool requestRTGI, bool requestPT, bool profileRTGI)
{
    requested = requestPT ? LightingMode::ReferencePT : (requestRTGI ? LightingMode::RTGI : LightingMode::Raster);
    effective = requested;
    scheduled = LightingMode::Raster;
    fallback = LightingFallback::None;
    conflictingRequests = requestRTGI && requestPT;
    opaqueScheduled = false;
    frameFailed = false;
    recoveryActive = false;
    failureCleared = false;
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
    rayStaticDetailInstances = 0;
    rtgiProfile = requested == LightingMode::RTGI && profileRTGI;
    rawSignalsRecorded = false;
    sceneRevision = 0;
    poseRevision = 0;

    if (m_latchedReason == LightingFallback::None)
        return;
    if (m_latchedMode != requested || m_latchedRTGIProfile != rtgiProfile)
    {
        ResetRecovery();
        return;
    }
    effective = LightingMode::Raster;
    recoveryActive = true;
    fallback = m_latchedReason;
}

void LightingFrameState::ScheduleOpaqueLighting()
{
    scheduled = effective;
    opaqueScheduled = true;
}

void LightingFrameState::Fail(LightingFallback reason)
{
    if (opaqueScheduled && scheduled != LightingMode::Raster)
    {
        if (frameFailed)
            return;
        if (reason == LightingFallback::None)
            reason = LightingFallback::RecordingUnavailable;
        frameFailed = true;
        effective = scheduled;
        fallback = reason;
        recorded = false;
        recordedSamples = 0;
        reuseAvailable = false;
        reuseReservoirs = false;
        historyUsed = false;
        rawSignalsRecorded = false;
        rayStaticDetailInstances = 0;
        m_latchedMode = requested;
        m_latchedReason = reason;
        m_latchedRTGIProfile = rtgiProfile;
        return;
    }

    effective = LightingMode::Raster;
    fallback = reason;
    recorded = false;
    recordedSamples = 0;
    reuseReservoirs = false;
    historyUsed = false;
    rawSignalsRecorded = false;
    rayStaticDetailInstances = 0;
}

void LightingFrameState::ResetRecovery()
{
    m_latchedMode = LightingMode::Raster;
    m_latchedReason = LightingFallback::None;
    m_latchedRTGIProfile = false;
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
    case LightingFallback::RecordingUnavailable:   return "RT dispatch recording unavailable";
    }
    return "unknown fallback";
}
}
