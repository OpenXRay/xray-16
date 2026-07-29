#define SM_5_0
#include "common.h"
#include "cull_utils.h"
#include "cull_debug.h"

struct GPUObjectData
{
    float3 position;
    float radius;
    uint batchIndex;
    uint flags;
    float2 padding;
};

cbuffer CullDebugParams : register(b5)
{
    float4x4 g_ViewProj;
    float4x4 g_PrevViewProj;
    float3 g_CameraPos;
    float g_MaxDistance;
    float4 g_FrustumPlanes[6];
    uint g_ObjectCount;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
    float g_OccluderThreshold;
    uint g_DebugOffset;
    float2 g_Padding;
}
;

StructuredBuffer<GPUObjectData> g_Objects : register(t0);

Texture2D<float> g_HiZPyramid : register(t1);


RWStructuredBuffer<CullDebugData> g_DebugOutput : register(u0);

float3 OcclusionTestSphereDebug(float3 center, float radius)
{
    HiZTestResult r = HiZTestSphereEx(center, radius, g_CameraPos, g_PrevViewProj,
                                      g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels);
    return float3(r.frontDepth, r.hiZDepth, r.visible ? 1.0 : 0.0);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint objectIdx = dtID.x;

if (objectIdx >= g_ObjectCount)
    return;

GPUObjectData obj = g_Objects[objectIdx];

CullDebugData debug;
debug.position = obj.position;
debug.radius = obj.radius;
debug.objectIndex = g_DebugOffset + objectIdx;
debug.objectDepth = 0.0;
debug.hiZDepth = 0.0;

if (!DistanceTestSphere(obj.position, obj.radius, g_CameraPos, g_MaxDistance))
{
    debug.cullState = CULL_STATE_CULLED_DISTANCE;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

if (!FrustumTestSphere(obj.position, obj.radius, g_FrustumPlanes))
{
    debug.cullState = CULL_STATE_CULLED_FRUSTUM;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

float3 occlusionResult = OcclusionTestSphereDebug(obj.position, obj.radius);
debug.objectDepth = occlusionResult.x;
debug.hiZDepth = occlusionResult.y;

if (occlusionResult.z < 0.5)
{
    debug.cullState = CULL_STATE_CULLED_OCCLUSION;
    g_DebugOutput[g_DebugOffset + objectIdx] = debug;
    return;
}

float3 delta = obj.position - g_CameraPos;
float distSq = dot(delta, delta);

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
