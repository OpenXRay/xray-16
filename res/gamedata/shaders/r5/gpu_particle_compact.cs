#include "gpu_particle_types.h"
cbuffer GpuPapiParams { GpuPapiConstants g_Papi; }
cbuffer GpuPapiVisibility { float4 g_FrustumPlanes[6]; }
StructuredBuffer<uint> g_VisibleRoots;
StructuredBuffer<GpuPapiParticle> g_Particles;
StructuredBuffer<GpuPapiEmitter> g_Emitters;
StructuredBuffer<GpuPapiProgram> g_Programs;
RWStructuredBuffer<uint> g_Indices;
RWStructuredBuffer<uint4> g_DrawArgs;
RWStructuredBuffer<uint> g_BucketOffsets;
RWStructuredBuffer<uint> g_BucketCursors;
groupshared uint g_LaneOffsets[64];
groupshared uint g_EmitterFirst;
uint GpuDrawBucket(GpuPapiEmitter emitter,GpuPapiProgram program) {
    bool hud = (emitter.flags & GPU_PAPI_HUD) != 0;
    return program.shaderVariant == 2 ? (hud ? 13u : 12u) : min(program.blendMode,5u) + (hud ? 6u : 0u);
}
float3 GpuCompactTransformDirection(GpuPapiEmitter emitter,float3 direction) {
    return emitter.basisX.xyz * direction.x + emitter.basisY.xyz * direction.y + emitter.basisZ.xyz * direction.z;
}
bool GpuParticleVisible(GpuPapiParticle particle,GpuPapiEmitter emitter,GpuPapiProgram program) {
    if ((emitter.flags & GPU_PAPI_HUD) != 0) return true;
    bool localSpace = (emitter.flags & GPU_PAPI_LOCAL) != 0;
    float3 position = particle.position;
    if (localSpace) position = GpuCompactTransformDirection(emitter,position) + emitter.origin.xyz;
    float2 halfSize = particle.size.xy * 0.5;
    float speed = 0.0;
    bool aligned = (program.flags & (1u << 15u)) != 0;
    if ((program.flags & ((1u << 18u) | (1u << 15u))) != 0) speed = length(particle.velocity);
    if ((program.flags & (1u << 18u)) != 0) halfSize += speed * program.velocityScale.xy;
    float radius = length(halfSize);
    bool transformedPlane = aligned && ((speed < 0.0000001 && (program.flags & (1u << 20u)) != 0) ||
        (speed >= 0.0000001 && (program.flags & (1u << 21u)) != 0));
    float3 top = 0.0;
    if (aligned && !transformedPlane) {
        if (speed >= 0.0000001) top = particle.velocity / speed;
        else {
            float2 sine,cosine;
            sincos(program.alignRotation.xy,sine,cosine);
            top = float3(cosine.x * sine.y,-sine.x,cosine.x * cosine.y);
        }
        if (localSpace) top = GpuCompactTransformDirection(emitter,top);
    }
    for (uint plane = 0; plane < 6; ++plane) {
        float4 p = g_FrustumPlanes[plane];
        float projectedScaleSquared = dot(p.xyz,p.xyz);
        if (transformedPlane && localSpace) {
            float3 projectedBasis = float3(dot(p.xyz,emitter.basisX.xyz),dot(p.xyz,emitter.basisY.xyz),
                dot(p.xyz,emitter.basisZ.xyz));
            projectedScaleSquared = dot(projectedBasis,projectedBasis);
        }
        else if (aligned && !transformedPlane) {
            float projectedTop = dot(p.xyz,top);
            projectedScaleSquared += projectedTop * projectedTop;
        }
        float extent = radius * sqrt(projectedScaleSquared);
        float distance = dot(p.xyz,position) + p.w;
        float tolerance = 0.00001 * (dot(abs(p.xyz),abs(position)) + abs(p.w) + extent + 1.0);
        if (distance > extent + tolerance) return false;
    }
    return true;
}
void GpuCompactEmitter(GpuPapiEmitter emitter,uint lane) {
    if (emitter.active != 1 || emitter.count == 0) return;
    GpuPapiProgram program = g_Programs[emitter.program];
    if ((program.flags & 1u) == 0) return;
    uint bucket = GpuDrawBucket(emitter,program);
    uint laneSize = emitter.count / 64;
    uint remainder = emitter.count % 64;
    uint begin = lane * laneSize + min(lane,remainder);
    uint end = begin + laneSize + (lane < remainder ? 1u : 0u);
    bool hud = (emitter.flags & GPU_PAPI_HUD) != 0;
    uint visibleCount = end - begin;
    if (!hud) {
        visibleCount = 0;
        for (uint ordinal = begin; ordinal < end; ++ordinal)
            if (GpuParticleVisible(g_Particles[emitter.particleFirst + ordinal],emitter,program)) ++visibleCount;
    }
    g_LaneOffsets[lane] = visibleCount;
    GroupMemoryBarrierWithGroupSync();
    if (lane == 0) {
        uint total = 0;
        for (uint i = 0; i < 64; ++i) {
            uint count = g_LaneOffsets[i];
            if (g_Papi.phase != 0) g_LaneOffsets[i] = total;
            total += count;
        }
        g_EmitterFirst = 0;
        if (total != 0) {
            if (g_Papi.phase == 0) InterlockedAdd(g_DrawArgs[bucket].y,total);
            else InterlockedAdd(g_BucketCursors[bucket],total,g_EmitterFirst);
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (g_Papi.phase != 0 && visibleCount != 0) {
        uint first = g_EmitterFirst + g_LaneOffsets[lane];
        uint bucketOffset = g_BucketOffsets[bucket];
        uint bucketCount = g_DrawArgs[bucket].y;
        for (uint ordinal = begin; ordinal < end; ++ordinal) {
            uint particleIndex = emitter.particleFirst + ordinal;
            bool visible = true;
            if (!hud) visible = GpuParticleVisible(g_Particles[particleIndex],emitter,program);
            if (visible) {
                if (first < bucketCount && bucketOffset + first < g_Papi.particleCapacity)
                    g_Indices[bucketOffset + first] = particleIndex;
                ++first;
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
}
[numthreads(64,1,1)]
void main(uint3 groupID : SV_GroupID,uint lane : SV_GroupIndex) {
    if (g_Papi.phase == 1) {
        if (groupID.x != 0 || lane != 0) return;
        uint offset = 0;
        for (uint bucket = 0; bucket < 14; ++bucket) {
            uint count = min(g_DrawArgs[bucket].y,g_Papi.particleCapacity - offset);
            g_BucketOffsets[bucket] = offset;
            g_BucketCursors[bucket] = 0;
            g_DrawArgs[bucket] = uint4(6,count,0,0);
            offset += count;
        }
        return;
    }
    uint index = g_Papi.pad + groupID.x;
    if (index >= g_Papi.rootCapacity) return;
    GpuPapiEmitter emitter = g_Emitters[g_VisibleRoots[index]];
    if (emitter.active != 1) return;
    GpuCompactEmitter(emitter,lane);
    uint child = emitter.pad0;
    while (child != GPU_PAPI_INVALID) {
        GpuPapiEmitter childEmitter = g_Emitters[child];
        GpuCompactEmitter(childEmitter,lane);
        child = childEmitter.pad0;
    }
}
