// particle_cull.cs
// GPU particle culling: frustum + Hi-Z occlusion
// Outputs visible particle indices and count

#define THREAD_GROUP_SIZE 64

#include "cull_utils.h"

// Must match GPUParticleData in ParticlePassSetup.h (48 bytes)
struct ParticleData {
    float3 position;    // 12 bytes
    float rotation;     //  4 bytes
    float2 size;        //  8 bytes
    uint color;         //  4 bytes
    uint materialID;    //  4 bytes
    float2 uvMin;       //  8 bytes
    float2 uvMax;       //  8 bytes
};

// Use b5 to avoid collision with common.h buffers (b0, b1, b2, b3, b4)
cbuffer ParticleCullParams : register(b5) {
    float4x4 g_ViewProj;        // Current frame (for frustum + Hi-Z culling) - 64 bytes
    float4 g_FrustumPlanes[6];  // 96 bytes
    float4 g_CameraPos;         // 16 bytes
    float4 g_CameraTop;         // 16 bytes
    float4 g_CameraRight;       // 16 bytes
    uint g_ParticleCount;       //  4 bytes
    uint g_HiZWidth;            //  4 bytes
    uint g_HiZHeight;           //  4 bytes
    uint g_HiZMipLevels;        //  4 bytes
};

StructuredBuffer<ParticleData> g_ParticleData : register(t0);
Texture2D<float> g_HiZPyramid : register(t1);
SamplerState g_PointClampSampler : register(s0);

RWStructuredBuffer<uint> g_VisibleIndices : register(u0);
RWByteAddressBuffer g_VisibleCount : register(u1);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint particleIdx = dispatchThreadID.x;

    if (particleIdx >= g_ParticleCount)
        return;

    ParticleData p = g_ParticleData[particleIdx];

    // Bounding sphere radius (diagonal of billboard)
    float radius = length(p.size);

    // Frustum culling (returns true = visible)
    if (!FrustumTestSphere(p.position, radius, g_FrustumPlanes))
        return;

    // Hi-Z occlusion culling (returns true = visible)
    if (g_HiZMipLevels > 0) {
        if (!HiZTestSphere(
                p.position,
                radius,
                g_CameraPos.xyz,
                g_ViewProj,
                g_HiZPyramid,
                g_PointClampSampler,
                g_HiZWidth,
                g_HiZHeight,
                g_HiZMipLevels))
            return;
    }

    // Particle is visible - append to output
    uint visibleIdx;
    g_VisibleCount.InterlockedAdd(0, 1, visibleIdx);
    g_VisibleIndices[visibleIdx] = particleIdx;
}
