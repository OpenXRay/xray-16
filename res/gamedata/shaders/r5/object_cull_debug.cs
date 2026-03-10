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
    float3 result = float3(0.0, 0.0, 1.0);

    float4 clipPos = mul(g_ViewProj, float4(center, 1.0));

    if (clipPos.w <= 0.001)
        return float3(0.0, 1.0, 1.0);

    float3 ndc = clipPos.xyz / clipPos.w;

    float projScale = max(abs(g_ViewProj[0][0]), abs(g_ViewProj[1][1]));
    float2 ndcSize = float2(radius, radius) * projScale / clipPos.w;
    float2 minNDC = ndc.xy - ndcSize;
    float2 maxNDC = ndc.xy + ndcSize;

    if (any(minNDC > 1.0) || any(maxNDC < -1.0))
        return float3(1.0, 0.0, 0.0);

    float2 minUV = minNDC * 0.5 + 0.5;
    float2 maxUV = maxNDC * 0.5 + 0.5;
    minUV.y = 1.0 - minUV.y;
    maxUV.y = 1.0 - maxUV.y;
    float4 boxUV = float4(min(minUV, maxUV), max(minUV, maxUV));

    float boxWidth = (boxUV.z - boxUV.x) * float(g_HiZWidth);
    float boxHeight = (boxUV.w - boxUV.y) * float(g_HiZHeight);
    float mipLevel = floor(log2(max(1.0, max(boxWidth, boxHeight) * 0.5)));
    mipLevel = clamp(mipLevel, 0.0, float(g_HiZMipLevels - 1));

    float d1 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.x, boxUV.y), mipLevel);
    float d2 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.z, boxUV.y), mipLevel);
    float d3 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.x, boxUV.w), mipLevel);
    float d4 = g_HiZPyramid.SampleLevel(smp_nofilter, float2(boxUV.z, boxUV.w), mipLevel);
    float hiZDepth = max(max(d1, d2), max(d3, d4));

    float3 viewDir = normalize(center - g_CameraPos);
    float3 frontPoint = center - viewDir * radius;
    float4 frontClip = mul(g_ViewProj, float4(frontPoint, 1.0));

    if (frontClip.w <= 0.001)
        return float3(0.0, hiZDepth, 1.0);

    float objectDepth = saturate(frontClip.z / frontClip.w);

    float depthBias = 0.0001;
    bool visible = objectDepth <= (hiZDepth + depthBias);

    return float3(objectDepth, hiZDepth, visible ? 1.0 : 0.0);
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
