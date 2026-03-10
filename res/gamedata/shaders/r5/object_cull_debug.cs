// object_cull_debug.cs - Debug visualization compute shader
// Tests all objects and outputs culling state (does NOT filter - outputs ALL objects)
//
// Output is used by cull_debug.vs/ps to render color-coded bounding spheres
//
#define SM_5_0
#include "common.h"
#include "cull_utils.h"
#include "cull_debug.h"

// ═══════════════════════════════════════════════════════
//  DATA STRUCTURES (matches object_cull.cs)
// ═══════════════════════════════════════════════════════

struct GPUObjectData
{
    float3 position;    // World-space bounding sphere center
    float radius;       // Bounding sphere radius
    uint batchIndex;    // Index into original batch array
    uint flags;         // Object flags
    float2 padding;
};

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

cbuffer CullDebugParams : register(b5)
{
    float4x4 g_ViewProj;           // View-projection matrix
    float3 g_CameraPos;            // Camera world position
    float g_MaxDistance;           // Maximum render distance (squared)
    float4 g_FrustumPlanes[6];     // View frustum planes (world space)
    uint g_ObjectCount;            // Total objects to test
    uint g_HiZWidth;               // Hi-Z pyramid base width
    uint g_HiZHeight;              // Hi-Z pyramid base height
    uint g_HiZMipLevels;           // Number of Hi-Z mip levels
    float g_OccluderThreshold;     // Distance threshold for "occluder" classification
    uint g_DebugOffset;
    float2 g_Padding;
}
;

// ═══════════════════════════════════════════════════════
//  RESOURCES
// ═══════════════════════════════════════════════════════

// Input: All objects to test
StructuredBuffer<GPUObjectData> g_Objects : register(t0);

// Input: Hi-Z pyramid
Texture2D<float> g_HiZPyramid : register(t1);


// Output: Debug data for ALL objects (same count as input)
RWStructuredBuffer<CullDebugData> g_DebugOutput : register(u0);

// ═══════════════════════════════════════════════════════
//  OCCLUSION TEST (returns depth values for debug)
// ═══════════════════════════════════════════════════════

// Returns: x = object depth, y = hi-z depth, z = 1 if visible, 0 if occluded
float3 OcclusionTestSphereDebug(float3 center, float radius)
{
    float3 result = float3(0.0, 0.0, 1.0); // Default: visible

    // Project sphere center to clip space (use our cbuffer, not common.h's m_VP)
    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    // Behind camera - conservatively visible
    if (clipPos.w <= 0.001)
        return float3(0.0, 1.0, 1.0);

    float3 ndc = clipPos.xyz / clipPos.w;

    // Calculate screen-space bounding box
    float projScale = max(abs(g_ViewProj[0][0]), abs(g_ViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / clipPos.w;
    float2 minNDC = ndc.xy - ndcSize;
    float2 maxNDC = ndc.xy + ndcSize;

    // Completely outside screen
    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return float3(1.0, 0.0, 0.0); // Occluded (off-screen)

    // Convert to UV space
    float2 minUV = minNDC * 0.5 + 0.5;
    float2 maxUV = maxNDC * 0.5 + 0.5;
    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;
    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    // Calculate mip level
    float boxWidth = (boxUV.z - boxUV.x) * float(g_HiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(g_HiZHeight);
    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    // Sample Hi-Z at 4 corners
    float d1 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.z, boxUV.w), mipLevel);
    float hiZDepth = max(max(d1, d2), max(d3, d4));

    // Calculate object's front depth
    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;
    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));

    if (frontClip.w <= 0.001)
        return float3(0.0, hiZDepth, 1.0); // Near plane intersection - visible

    float objectDepth = saturate(frontClip.z / frontClip.w);

    // Depth comparison
    float depthBias = 0.0001;
    bool visible = objectDepth <= (hiZDepth + depthBias);

    return float3(objectDepth, hiZDepth, visible ? 1.0 : 0.0);
}

// ═══════════════════════════════════════════════════════
//  MAIN COMPUTE SHADER
// ═══════════════════════════════════════════════════════

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;

if (objectIdx >= g_ObjectCount)
    return;

GPUObjectData obj = g_Objects[objectIdx];

// Initialize debug output
CullDebugData debug;
debug.position = obj.position;
debug.radius = obj.radius;
debug.objectIndex = g_DebugOffset + objectIdx;
debug.objectDepth = 0.0;
debug.hiZDepth = 0.0;

// ─────────────────────────────────────────────────────
//  TEST 1: Distance culling
// ─────────────────────────────────────────────────────
if (!DistanceTestSphere(obj.position, obj.radius, g_CameraPos, g_MaxDistance))
{
    debug.cullState = CULL_STATE_CULLED_DISTANCE;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

// ─────────────────────────────────────────────────────
//  TEST 2: Frustum culling
// ─────────────────────────────────────────────────────
if (!FrustumTestSphere(obj.position, obj.radius, g_FrustumPlanes))
{
    debug.cullState = CULL_STATE_CULLED_FRUSTUM;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

// ─────────────────────────────────────────────────────
//  TEST 3: Hi-Z occlusion culling
// ─────────────────────────────────────────────────────
float3 occlusionResult = OcclusionTestSphereDebug(obj.position, obj.radius);
debug.objectDepth = occlusionResult.x;
debug.hiZDepth = occlusionResult.y;

if (occlusionResult.z < 0.5) // Occluded
{
    debug.cullState = CULL_STATE_CULLED_OCCLUSION;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

// ─────────────────────────────────────────────────────
//  PASSED ALL TESTS - Determine if occluder or just visible
// ─────────────────────────────────────────────────────
float3 delta = obj.position - g_CameraPos;
float distSq = dot(delta, delta);

// Objects close to camera are potential occluders (contributing to Hi-Z)
if (distSq < g_OccluderThreshold * g_OccluderThreshold)
{
    debug.cullState = CULL_STATE_OCCLUDER;
}
else
{
    debug.cullState = CULL_STATE_VISIBLE;
}

g_DebugOutput[g_DebugOffset + objectIdx] = debug;
}
