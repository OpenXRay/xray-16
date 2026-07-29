#define SM_5_0
#include "common.h"
#include "cull_utils.h"
#include "cull_debug.h"

struct GPUParticleData
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
    uint g_ParticleCount;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
    float g_OccluderThreshold;
    uint g_DebugOffset;
    float2 g_Padding;
};

StructuredBuffer<GPUParticleData> g_Particles : register(t0);
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
    uint particleIdx = dtID.x;

    if (particleIdx >= g_ParticleCount)
        return;

    GPUParticleData particle = g_Particles[particleIdx];

    CullDebugData debug;
    debug.position = particle.position;
    debug.radius = particle.radius;
    debug.objectIndex = particleIdx;
    debug.objectDepth = 0.0;
    debug.hiZDepth = 0.0;

    bool culled = false;

    if (!DistanceTestSphere(particle.position, particle.radius, g_CameraPos, g_MaxDistance))
    {
        culled = true;
    }
    else if (!FrustumTestSphere(particle.position, particle.radius, g_FrustumPlanes))
    {
        culled = true;
    }
    else
    {
        float3 occlusionResult = OcclusionTestSphereDebug(particle.position, particle.radius);
        debug.objectDepth = occlusionResult.x;
        debug.hiZDepth = occlusionResult.y;

        if (occlusionResult.z < 0.5)
            culled = true;
    }

    debug.cullState = culled ? CULL_STATE_PARTICLE_CULLED : CULL_STATE_PARTICLE_VISIBLE;
    g_DebugOutput[g_DebugOffset + particleIdx] = debug;
}
