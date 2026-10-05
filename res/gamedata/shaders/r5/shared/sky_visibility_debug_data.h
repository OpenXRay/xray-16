#ifndef SKY_VISIBILITY_DEBUG_DATA_H
#define SKY_VISIBILITY_DEBUG_DATA_H

static const uint SKY_PROBE_DEBUG_VERSION = 2u;
static const uint SKY_PROBE_DEBUG_PRODUCTION_MOMENTS = 1u;
static const uint SKY_PROBE_DEBUG_HEADER_ROWS = 20u;
static const uint SKY_PROBE_DEBUG_CORNER_ROWS = 24u;
static const uint SKY_PROBE_DEBUG_ROWS = SKY_PROBE_DEBUG_HEADER_ROWS + 8u * SKY_PROBE_DEBUG_CORNER_ROWS;
static const uint SKY_PROBE_DEBUG_ROW_POSITION = 6u;
static const uint SKY_PROBE_DEBUG_ROW_WEIGHTS = 9u;
static const uint SKY_PROBE_DEBUG_ROW_CONTRIBUTION = 11u;

struct SkyProbeDebugRay
{
    float4 metrics;
    uint4 hit;
};

struct SkyProbeDebugCorner
{
    uint4 identity;
    uint4 cellAndFlags;
    uint4 rawVisibility;
    uint4 rawDepthMean;
    uint4 rawDepthSigma;
    float4 moment;
    float4 position;
    float4 offset;
    float4 coefficients;
    float4 weights;
    float4 sky;
    float4 contribution;
    SkyProbeDebugRay rays[6];
};

struct SkyProbeDebugSnapshot
{
    uint4 metadata;
    uint4 selection;
    uint4 gridDims;
    float4 gridOrigin;
    float4 camera;
    float4 surface;
    float4 shadingNormal;
    float4 geometricNormal;
    float4 material;
    float4 view;
    float4 biased;
    float4 rayOrigin;
    float4 controlOrigin;
    float4 sampleSH;
    float4 summary;
    float4 sky;
    float4 reflection;
    float4 settings;
    float4 lighting;
    uint4 scene;
    SkyProbeDebugCorner corners[8];
};

#ifdef SKY_PROBE_DIAGNOSTICS
cbuffer SkyVisibilityInspectParams : register(b5)
{
    uint4 g_SkyInspectCapture;
    float4 g_SkyInspectSettings;
    float4 g_SkyInspectLighting;
    uint4 g_SkyInspectScene;
};
#endif

#endif
