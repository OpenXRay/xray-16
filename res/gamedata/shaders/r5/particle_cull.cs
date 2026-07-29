#define THREAD_GROUP_SIZE 64

#include "common_samplers.h"
#include "cull_utils.h"

struct ParticleData {
    float3 position;
    float radius;
    uint batchIndex;
    uint flags;
    float pad0;
    float pad1;
};

struct IndirectDrawArgs {
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

cbuffer ParticleCullParams : register(b5) {
    float4x4 g_PrevViewProj;
    float4 g_FrustumPlanes[6];
    float4 g_CameraPos;
    uint g_SlotCount;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
};

StructuredBuffer<ParticleData> g_ParticleData : register(t0);
Texture2D<float> g_HiZPyramid : register(t1);

RWStructuredBuffer<IndirectDrawArgs> g_DrawArgs : register(u0);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint slot = dispatchThreadID.x;

    if (slot >= g_SlotCount)
        return;

    ParticleData p = g_ParticleData[slot];

    bool visible = FrustumTestSphere(p.position, p.radius, g_FrustumPlanes);

    if (visible && g_HiZMipLevels > 0)
        visible = HiZTestSphere(p.position, p.radius, g_CameraPos.xyz, g_PrevViewProj,
                                g_HiZPyramid, smp_nofilter, g_HiZWidth, g_HiZHeight, g_HiZMipLevels);

    g_DrawArgs[slot].instanceCount = visible ? 1u : 0u;
}
