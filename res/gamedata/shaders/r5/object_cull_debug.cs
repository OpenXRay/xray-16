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
    float4x4 g_PrevViewProj;       // Previous frame (for Hi-Z sampling)
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
    HiZTestResult r = HiZTestSphereEx(center, radius, g_CameraPos, g_PrevViewProj,
                                      g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels);
    return float3(r.frontDepth, r.hiZDepth, r.visible ? 1.0 : 0.0);
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
