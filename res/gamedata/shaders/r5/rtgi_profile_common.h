#ifndef RTGI_PROFILE_COMMON_H
#define RTGI_PROFILE_COMMON_H

#include "rtgi_trace_common.h"

cbuffer RTGIProfileParams : register(b6)
{
    uint g_ProfileTileY;
    uint g_ProfileTileHeight;
    uint g_ProfileSampleIndex;
    uint g_ProfilePad;
};

RWByteAddressBuffer u_ProfilePaths : register(u8);
RWByteAddressBuffer u_ProfileHits : register(u9);
RWByteAddressBuffer u_ProfileSums : register(u10);

static const uint RTGI_PROFILE_PATH_STRIDE = 304u;
static const uint RTGI_PROFILE_HIT_STRIDE = 96u;
static const uint RTGI_PROFILE_SUM_STRIDE = 48u;

static const uint RTGI_PROFILE_OFFSET_SETTINGS = 0u;
static const uint RTGI_PROFILE_OFFSET_RADIANCE = 16u;
static const uint RTGI_PROFILE_OFFSET_ALBEDO = 32u;
static const uint RTGI_PROFILE_OFFSET_NORMAL = 48u;
static const uint RTGI_PROFILE_OFFSET_DIRECT_DIFFUSE = 64u;
static const uint RTGI_PROFILE_OFFSET_DIRECT_SPECULAR = 80u;
static const uint RTGI_PROFILE_OFFSET_INDIRECT_DIFFUSE = 96u;
static const uint RTGI_PROFILE_OFFSET_INDIRECT_SPECULAR = 112u;
static const uint RTGI_PROFILE_OFFSET_EMISSION = 128u;
static const uint RTGI_PROFILE_OFFSET_COVERAGE = 144u;
static const uint RTGI_PROFILE_OFFSET_ORIGIN = 160u;
static const uint RTGI_PROFILE_OFFSET_DIRECTION = 176u;
static const uint RTGI_PROFILE_OFFSET_THROUGHPUT = 192u;
static const uint RTGI_PROFILE_OFFSET_DIFFUSE_THROUGHPUT = 208u;
static const uint RTGI_PROFILE_OFFSET_SPECULAR_THROUGHPUT = 224u;
static const uint RTGI_PROFILE_OFFSET_PREVIOUS_POSITION = 240u;
static const uint RTGI_PROFILE_OFFSET_COUNTERS = 256u;
static const uint RTGI_PROFILE_OFFSET_PARTIAL_DIFFUSE = 272u;
static const uint RTGI_PROFILE_OFFSET_PARTIAL_SPECULAR = 288u;

static const uint RTGI_PROFILE_SETTING_DIAGNOSTICS = 0x1u;
static const uint RTGI_PROFILE_SETTING_SEGMENT_METRICS = 0x2u;
static const uint RTGI_PROFILE_SETTING_HUD_FIRST_RAY = 0x4u;

static const uint RTGI_PROFILE_RESULT_INVALID = 0x1u;
static const uint RTGI_PROFILE_RESULT_FIRST_SEGMENT = 0x2u;
static const uint RTGI_PROFILE_RESULT_VALID = 0x4u;

static const uint RTGI_PROFILE_STATE_PREVIOUS_DELTA = 0x1u;
static const uint RTGI_PROFILE_STATE_FIRST_RAY = 0x2u;
static const uint RTGI_PROFILE_STATE_FIRST_SURFACE = 0x4u;
static const uint RTGI_PROFILE_STATE_PENDING_SEGMENT = 0x8u;
static const uint RTGI_PROFILE_STATE_ACTIVE = 0x10u;
static const uint RTGI_PROFILE_STATE_VALID_PRIMARY = 0x20u;

static const uint RTGI_PROFILE_TRACE_BATCH = 0u;
static const uint RTGI_PROFILE_TRACE_PRIMITIVE = 4u;
static const uint RTGI_PROFILE_TRACE_BARYCENTRICS = 8u;
static const uint RTGI_PROFILE_TRACE_DISTANCE = 16u;
static const uint RTGI_PROFILE_TRACE_CONE_WIDTH = 20u;
static const uint RTGI_PROFILE_TRACE_CONE_SPREAD = 24u;
static const uint RTGI_PROFILE_TRACE_FLAGS = 28u;
static const uint RTGI_PROFILE_TRACE_ROW0 = 32u;
static const uint RTGI_PROFILE_TRACE_ROW1 = 48u;
static const uint RTGI_PROFILE_TRACE_ROW2 = 64u;
static const uint RTGI_PROFILE_TRACE_POSITION = 80u;
static const uint RTGI_PROFILE_TRACE_FRONT_FACE = 0x1u;

static const uint RTGI_PROFILE_MATERIAL_ALBEDO = 0u;
static const uint RTGI_PROFILE_MATERIAL_NORMAL = 16u;
static const uint RTGI_PROFILE_MATERIAL_EMISSIVE = 32u;
static const uint RTGI_PROFILE_MATERIAL_GEO_NORMAL = 48u;
static const uint RTGI_PROFILE_MATERIAL_POSITION = 64u;
static const uint RTGI_PROFILE_MATERIAL_CLASS = 80u;

static const uint RTGI_PROFILE_SUM_DIFFUSE = 0u;
static const uint RTGI_PROFILE_SUM_SPECULAR = 16u;
static const uint RTGI_PROFILE_SUM_WEIGHTS = 32u;

void RTGIProfileStoreFloat(RWByteAddressBuffer buffer, uint offset, float value)
{
    buffer.Store(offset, asuint(value));
}

float RTGIProfileLoadFloat(RWByteAddressBuffer buffer, uint offset)
{
    return asfloat(buffer.Load(offset));
}

void RTGIProfileStoreFloat3(RWByteAddressBuffer buffer, uint offset, float3 value)
{
    buffer.Store3(offset, uint3(asuint(value.x), asuint(value.y), asuint(value.z)));
}

float3 RTGIProfileLoadFloat3(RWByteAddressBuffer buffer, uint offset)
{
    uint3 value = buffer.Load3(offset);
    return float3(asfloat(value.x), asfloat(value.y), asfloat(value.z));
}

struct RTGIProfileLane
{
    uint2 pixel;
    uint lane;
    uint pathBase;
    uint hitBase;
    uint sumBase;
};

bool RTGIProfileResolveLane(uint3 dispatchID, out RTGIProfileLane lane)
{
    uint width = uint(g_ScreenWidth);
    if (dispatchID.x >= width || dispatchID.y >= g_ProfileTileHeight)
        return false;
    if (dispatchID.y + g_ProfileTileY >= uint(g_ScreenHeight))
        return false;
    lane.pixel = uint2(dispatchID.x, dispatchID.y + g_ProfileTileY);
    lane.lane = dispatchID.x + dispatchID.y * width;
    lane.pathBase = lane.lane * RTGI_PROFILE_PATH_STRIDE;
    lane.hitBase = lane.lane * RTGI_PROFILE_HIT_STRIDE;
    lane.sumBase = lane.lane * RTGI_PROFILE_SUM_STRIDE;
    return true;
}

void RTGIProfileStoreState(RWByteAddressBuffer buffer, uint base, RTIntegratorState state,
    bool active, bool validPrimary, uint rng)
{
    buffer.Store4(base + RTGI_PROFILE_OFFSET_SETTINGS, uint4(state.settings.maxBounces,
        asuint(state.settings.coneWidth), asuint(state.settings.coneSpread),
        (state.settings.trackDiagnostics ? RTGI_PROFILE_SETTING_DIAGNOSTICS : 0u) |
        (state.settings.trackSegmentMetrics ? RTGI_PROFILE_SETTING_SEGMENT_METRICS : 0u) |
        (state.settings.allowHudFirstRay ? RTGI_PROFILE_SETTING_HUD_FIRST_RAY : 0u)));

    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_RADIANCE, state.result.radiance);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_RADIANCE + 12u, state.result.roughness);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_ALBEDO, state.result.albedo);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_ALBEDO + 12u, state.result.metallic);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_NORMAL, state.result.normal);
    buffer.Store(base + RTGI_PROFILE_OFFSET_NORMAL + 12u, state.result.pathLength);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_DIFFUSE, state.result.directDiffuse);
    buffer.Store(base + RTGI_PROFILE_OFFSET_DIRECT_DIFFUSE + 12u, state.result.scatteringDepth);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_SPECULAR, state.result.directSpecular);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_SPECULAR + 12u, state.result.firstSegmentDistance);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_DIFFUSE, state.result.indirectDiffuse);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_DIFFUSE + 12u, state.result.firstDiffuseShare);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_SPECULAR, state.result.indirectSpecular);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_SPECULAR + 12u, state.result.firstSpecularShare);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_EMISSION, state.result.emission);
    buffer.Store(base + RTGI_PROFILE_OFFSET_EMISSION + 12u,
        (state.result.invalid ? RTGI_PROFILE_RESULT_INVALID : 0u) |
        (state.result.firstSegmentDefined ? RTGI_PROFILE_RESULT_FIRST_SEGMENT : 0u) |
        (state.result.valid ? RTGI_PROFILE_RESULT_VALID : 0u));
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_COVERAGE, state.result.coverage);
    buffer.Store(base + RTGI_PROFILE_OFFSET_COVERAGE + 12u, 0u);

    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_ORIGIN, state.origin);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_ORIGIN + 12u, state.remainingReach);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECTION, state.direction);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_DIRECTION + 12u, state.previousPdf);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_THROUGHPUT, state.throughput);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_THROUGHPUT + 12u, state.coneWidth);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIFFUSE_THROUGHPUT, state.diffuseThroughput);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_DIFFUSE_THROUGHPUT + 12u, state.coneSpread);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_SPECULAR_THROUGHPUT, state.specularThroughput);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_SPECULAR_THROUGHPUT + 12u, state.diffuseLobeShare);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_PREVIOUS_POSITION, state.previousPosition);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_OFFSET_PREVIOUS_POSITION + 12u, state.specularLobeShare);

    uint stateFlags = (state.previousDelta ? RTGI_PROFILE_STATE_PREVIOUS_DELTA : 0u) |
        (state.firstRayTraced ? RTGI_PROFILE_STATE_FIRST_RAY : 0u) |
        (state.firstSurfaceRecorded ? RTGI_PROFILE_STATE_FIRST_SURFACE : 0u) |
        (state.pendingFirstSegment ? RTGI_PROFILE_STATE_PENDING_SEGMENT : 0u) |
        (active ? RTGI_PROFILE_STATE_ACTIVE : 0u) |
        (validPrimary ? RTGI_PROFILE_STATE_VALID_PRIMARY : 0u);
    buffer.Store4(base + RTGI_PROFILE_OFFSET_COUNTERS, uint4(state.bounces, state.nullEvents, stateFlags, rng));
}

RTIntegratorState RTGIProfileLoadState(RWByteAddressBuffer buffer, uint base, out uint rng,
    out bool active, out bool validPrimary)
{
    RTIntegratorState state;
    state.settings = RTIntegratorDefaultSettings();
    uint4 settings = buffer.Load4(base + RTGI_PROFILE_OFFSET_SETTINGS);
    state.settings.maxBounces = settings.x;
    state.settings.coneWidth = asfloat(settings.y);
    state.settings.coneSpread = asfloat(settings.z);
    state.settings.trackDiagnostics = (settings.w & RTGI_PROFILE_SETTING_DIAGNOSTICS) != 0u;
    state.settings.trackSegmentMetrics = (settings.w & RTGI_PROFILE_SETTING_SEGMENT_METRICS) != 0u;
    state.settings.allowHudFirstRay = (settings.w & RTGI_PROFILE_SETTING_HUD_FIRST_RAY) != 0u;

    state.result = (RTIntegratorResult)0;
    state.result.radiance = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_RADIANCE);
    state.result.roughness = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_RADIANCE + 12u);
    state.result.albedo = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_ALBEDO);
    state.result.metallic = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_ALBEDO + 12u);
    state.result.normal = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_NORMAL);
    state.result.pathLength = buffer.Load(base + RTGI_PROFILE_OFFSET_NORMAL + 12u);
    state.result.directDiffuse = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_DIFFUSE);
    state.result.scatteringDepth = buffer.Load(base + RTGI_PROFILE_OFFSET_DIRECT_DIFFUSE + 12u);
    state.result.directSpecular = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_SPECULAR);
    state.result.firstSegmentDistance = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_DIRECT_SPECULAR + 12u);
    state.result.indirectDiffuse = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_DIFFUSE);
    state.result.firstDiffuseShare = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_DIFFUSE + 12u);
    state.result.indirectSpecular = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_SPECULAR);
    state.result.firstSpecularShare = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_INDIRECT_SPECULAR + 12u);
    state.result.emission = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_EMISSION);
    uint resultFlags = buffer.Load(base + RTGI_PROFILE_OFFSET_EMISSION + 12u);
    state.result.invalid = (resultFlags & RTGI_PROFILE_RESULT_INVALID) != 0u;
    state.result.firstSegmentDefined = (resultFlags & RTGI_PROFILE_RESULT_FIRST_SEGMENT) != 0u;
    state.result.valid = (resultFlags & RTGI_PROFILE_RESULT_VALID) != 0u;
    state.result.coverage = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_COVERAGE);

    state.origin = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_ORIGIN);
    state.remainingReach = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_ORIGIN + 12u);
    state.direction = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIRECTION);
    state.previousPdf = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_DIRECTION + 12u);
    state.throughput = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_THROUGHPUT);
    state.coneWidth = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_THROUGHPUT + 12u);
    state.diffuseThroughput = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_DIFFUSE_THROUGHPUT);
    state.coneSpread = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_DIFFUSE_THROUGHPUT + 12u);
    state.specularThroughput = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_SPECULAR_THROUGHPUT);
    state.diffuseLobeShare = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_SPECULAR_THROUGHPUT + 12u);
    state.previousPosition = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_PREVIOUS_POSITION);
    state.specularLobeShare = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_OFFSET_PREVIOUS_POSITION + 12u);

    uint4 counters = buffer.Load4(base + RTGI_PROFILE_OFFSET_COUNTERS);
    state.bounces = counters.x;
    state.nullEvents = counters.y;
    state.previousDelta = (counters.z & RTGI_PROFILE_STATE_PREVIOUS_DELTA) != 0u;
    state.firstRayTraced = (counters.z & RTGI_PROFILE_STATE_FIRST_RAY) != 0u;
    state.firstSurfaceRecorded = (counters.z & RTGI_PROFILE_STATE_FIRST_SURFACE) != 0u;
    state.pendingFirstSegment = (counters.z & RTGI_PROFILE_STATE_PENDING_SEGMENT) != 0u;
    rng = counters.w;
    active = (counters.z & RTGI_PROFILE_STATE_ACTIVE) != 0u;
    validPrimary = (counters.z & RTGI_PROFILE_STATE_VALID_PRIMARY) != 0u;
    return state;
}

void RTGIProfileStoreDirectTerms(RWByteAddressBuffer buffer, uint base, RTDirectTerms terms)
{
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_PARTIAL_DIFFUSE, terms.diffuse);
    buffer.Store(base + RTGI_PROFILE_OFFSET_PARTIAL_DIFFUSE + 12u, terms.invalid ? 1u : 0u);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_OFFSET_PARTIAL_SPECULAR, terms.specular);
    buffer.Store(base + RTGI_PROFILE_OFFSET_PARTIAL_SPECULAR + 12u, 0u);
}

RTDirectTerms RTGIProfileLoadDirectTerms(RWByteAddressBuffer buffer, uint base)
{
    RTDirectTerms terms = (RTDirectTerms)0;
    terms.diffuse = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_PARTIAL_DIFFUSE);
    terms.invalid = buffer.Load(base + RTGI_PROFILE_OFFSET_PARTIAL_DIFFUSE + 12u) != 0u;
    terms.specular = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_OFFSET_PARTIAL_SPECULAR);
    return terms;
}

void RTGIProfileStoreTransform(RWByteAddressBuffer buffer, uint offset, float3x4 transform)
{
    buffer.Store4(offset, uint4(asuint(transform[0][0]), asuint(transform[0][1]),
        asuint(transform[0][2]), asuint(transform[0][3])));
    buffer.Store4(offset + 16u, uint4(asuint(transform[1][0]), asuint(transform[1][1]),
        asuint(transform[1][2]), asuint(transform[1][3])));
    buffer.Store4(offset + 32u, uint4(asuint(transform[2][0]), asuint(transform[2][1]),
        asuint(transform[2][2]), asuint(transform[2][3])));
}

void RTGIProfileStoreTraceHit(RWByteAddressBuffer buffer, uint base, RTSceneTrace trace, float3 hitPosition)
{
    buffer.Store2(base + RTGI_PROFILE_TRACE_BATCH, uint2(trace.batchIdx, trace.primitiveIndex));
    buffer.Store2(base + RTGI_PROFILE_TRACE_BARYCENTRICS, asuint(trace.barycentrics));
    buffer.Store(base + RTGI_PROFILE_TRACE_DISTANCE, asuint(trace.t));
    buffer.Store(base + RTGI_PROFILE_TRACE_CONE_WIDTH, asuint(trace.coneWidth));
    buffer.Store(base + RTGI_PROFILE_TRACE_CONE_SPREAD, asuint(trace.coneSpread));
    buffer.Store(base + RTGI_PROFILE_TRACE_FLAGS, trace.frontFace ? RTGI_PROFILE_TRACE_FRONT_FACE : 0u);
    RTGIProfileStoreTransform(buffer, base + RTGI_PROFILE_TRACE_ROW0, trace.objectToWorld);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_TRACE_POSITION, hitPosition);
    buffer.Store(base + RTGI_PROFILE_TRACE_POSITION + 12u, 0u);
}

RTSceneTrace RTGIProfileLoadTraceHit(RWByteAddressBuffer buffer, uint base, out float3 hitPosition)
{
    RTSceneTrace trace = (RTSceneTrace)0;
    uint2 ids = buffer.Load2(base + RTGI_PROFILE_TRACE_BATCH);
    trace.batchIdx = ids.x;
    trace.primitiveIndex = ids.y;
    trace.info = g_BatchInfo[trace.batchIdx];
    trace.barycentrics = asfloat(buffer.Load2(base + RTGI_PROFILE_TRACE_BARYCENTRICS));
    trace.t = asfloat(buffer.Load(base + RTGI_PROFILE_TRACE_DISTANCE));
    trace.coneWidth = asfloat(buffer.Load(base + RTGI_PROFILE_TRACE_CONE_WIDTH));
    trace.coneSpread = asfloat(buffer.Load(base + RTGI_PROFILE_TRACE_CONE_SPREAD));
    trace.frontFace = (buffer.Load(base + RTGI_PROFILE_TRACE_FLAGS) & RTGI_PROFILE_TRACE_FRONT_FACE) != 0u;
    trace.objectToWorld = float3x4(asfloat(buffer.Load4(base + RTGI_PROFILE_TRACE_ROW0)),
        asfloat(buffer.Load4(base + RTGI_PROFILE_TRACE_ROW1)),
        asfloat(buffer.Load4(base + RTGI_PROFILE_TRACE_ROW2)));
    trace.hit = true;
    trace.transmittance = 1.0;
    hitPosition = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_TRACE_POSITION);
    return trace;
}

void RTGIProfileStoreMaterialHit(RWByteAddressBuffer buffer, uint base, RTHitSurface hit,
    RTHitGeometry geometry, float3 hitPosition, float segmentDistance)
{
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_MATERIAL_ALBEDO, hit.surface.albedo);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_MATERIAL_ALBEDO + 12u, hit.surface.roughness);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_MATERIAL_NORMAL, hit.surface.N);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_MATERIAL_NORMAL + 12u, hit.surface.metallic);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_MATERIAL_EMISSIVE, hit.surface.emissive);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_MATERIAL_EMISSIVE + 12u, hit.surface.ao);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_MATERIAL_GEO_NORMAL, geometry.geoNormal);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_MATERIAL_GEO_NORMAL + 12u, hit.surface.transmission);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_MATERIAL_POSITION, hitPosition);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_MATERIAL_POSITION + 12u, segmentDistance);
    buffer.Store4(base + RTGI_PROFILE_MATERIAL_CLASS,
        uint4(hit.surface.shadingClass, hit.flags, 0u, 0u));
}

void RTGIProfileLoadMaterialHit(RWByteAddressBuffer buffer, uint base, out RTHitSurface hit,
    out RTHitGeometry geometry, out float3 hitPosition, out float segmentDistance)
{
    hit = (RTHitSurface)0;
    hit.surface.albedo = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_MATERIAL_ALBEDO);
    hit.surface.roughness = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_MATERIAL_ALBEDO + 12u);
    hit.surface.N = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_MATERIAL_NORMAL);
    hit.surface.metallic = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_MATERIAL_NORMAL + 12u);
    hit.surface.emissive = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_MATERIAL_EMISSIVE);
    hit.surface.ao = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_MATERIAL_EMISSIVE + 12u);
    float3 geoNormal = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_MATERIAL_GEO_NORMAL);
    hit.surface.transmission = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_MATERIAL_GEO_NORMAL + 12u);
    hitPosition = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_MATERIAL_POSITION);
    segmentDistance = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_MATERIAL_POSITION + 12u);
    uint4 classification = buffer.Load4(base + RTGI_PROFILE_MATERIAL_CLASS);
    hit.surface.shadingClass = classification.x;
    hit.flags = classification.y;
    geometry = (RTHitGeometry)0;
    geometry.geoNormal = geoNormal;
}

void RTGIProfileClearSums(RWByteAddressBuffer buffer, uint base)
{
    buffer.Store4(base + RTGI_PROFILE_SUM_DIFFUSE, uint4(0u, 0u, 0u, 0u));
    buffer.Store4(base + RTGI_PROFILE_SUM_DIFFUSE + 16u, uint4(0u, 0u, 0u, 0u));
    buffer.Store4(base + RTGI_PROFILE_SUM_DIFFUSE + 32u, uint4(0u, 0u, 0u, 0u));
}

RTGIAccumulation RTGIProfileLoadSums(RWByteAddressBuffer buffer, uint base)
{
    RTGIAccumulation accumulation;
    accumulation.diffuseSum = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_SUM_DIFFUSE);
    accumulation.diffuseDistanceSum = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_SUM_DIFFUSE + 12u);
    accumulation.specularSum = RTGIProfileLoadFloat3(buffer, base + RTGI_PROFILE_SUM_SPECULAR);
    accumulation.specularDistanceSum = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_SUM_SPECULAR + 12u);
    accumulation.diffuseWeightSum = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_SUM_WEIGHTS);
    accumulation.specularWeightSum = RTGIProfileLoadFloat(buffer, base + RTGI_PROFILE_SUM_WEIGHTS + 4u);
    accumulation.validCount = buffer.Load(base + RTGI_PROFILE_SUM_WEIGHTS + 8u);
    accumulation.depthSum = buffer.Load(base + RTGI_PROFILE_SUM_WEIGHTS + 12u);
    return accumulation;
}

void RTGIProfileStoreSums(RWByteAddressBuffer buffer, uint base, RTGIAccumulation accumulation)
{
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_SUM_DIFFUSE, accumulation.diffuseSum);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_SUM_DIFFUSE + 12u, accumulation.diffuseDistanceSum);
    RTGIProfileStoreFloat3(buffer, base + RTGI_PROFILE_SUM_SPECULAR, accumulation.specularSum);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_SUM_SPECULAR + 12u, accumulation.specularDistanceSum);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_SUM_WEIGHTS, accumulation.diffuseWeightSum);
    RTGIProfileStoreFloat(buffer, base + RTGI_PROFILE_SUM_WEIGHTS + 4u, accumulation.specularWeightSum);
    buffer.Store(base + RTGI_PROFILE_SUM_WEIGHTS + 8u, accumulation.validCount);
    buffer.Store(base + RTGI_PROFILE_SUM_WEIGHTS + 12u, accumulation.depthSum);
}

#endif
