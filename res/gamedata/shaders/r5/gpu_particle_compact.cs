#include "gpu_particle_types.h"
cbuffer GpuPapiParams { GpuPapiConstants g_Papi; }
cbuffer GpuPapiVisibility { float4 g_FrustumPlanes[6]; }
StructuredBuffer<uint> g_VisibleRoots;
StructuredBuffer<GpuPapiEmitter> g_Emitters;
StructuredBuffer<GpuPapiProgram> g_Programs;
RWStructuredBuffer<uint> g_Indices;
RWStructuredBuffer<uint4> g_DrawArgs;
RWStructuredBuffer<uint> g_BucketOffsets;
RWStructuredBuffer<uint> g_BucketCursors;
uint GpuDrawBucket(GpuPapiEmitter emitter,GpuPapiProgram program) {
    bool hud = (emitter.flags & GPU_PAPI_HUD) != 0;
    return program.shaderVariant == 2 ? (hud ? 13u : 12u) : min(program.blendMode,5u) + (hud ? 6u : 0u);
}
bool GpuRootVisible(GpuPapiEmitter emitter) {
    if ((emitter.flags & GPU_PAPI_HUD) != 0) return true;
    for (uint plane = 0; plane < 6; ++plane) {
        float4 p = g_FrustumPlanes[plane];
        float3 nearest = float3(p.x >= 0 ? emitter.boundsMin.x : emitter.boundsMax.x,
            p.y >= 0 ? emitter.boundsMin.y : emitter.boundsMax.y,
            p.z >= 0 ? emitter.boundsMin.z : emitter.boundsMax.z);
        if (dot(p.xyz,nearest) + p.w > 0) return false;
    }
    return true;
}
void GpuCompactEmitter(GpuPapiEmitter emitter) {
    if (emitter.active != 1 || emitter.count == 0) return;
    GpuPapiProgram program = g_Programs[emitter.program];
    if ((program.flags & 1u) == 0) return;
    uint bucket = GpuDrawBucket(emitter,program);
    if (g_Papi.phase == 0) InterlockedAdd(g_DrawArgs[bucket].y,emitter.count);
    else {
        uint first;
        InterlockedAdd(g_BucketCursors[bucket],emitter.count,first);
        first += g_BucketOffsets[bucket];
        for (uint i = 0; i < emitter.count; ++i) g_Indices[first + i] = emitter.particleFirst + i;
    }
}
[numthreads(64,1,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    uint index = dispatchID.x;
    if (g_Papi.phase == 1) {
        if (index != 0) return;
        uint offset = 0;
        for (uint bucket = 0; bucket < 14; ++bucket) {
            uint count = g_DrawArgs[bucket].y;
            g_BucketOffsets[bucket] = offset;
            g_BucketCursors[bucket] = 0;
            g_DrawArgs[bucket] = uint4(6,count,bucket * 6u,0);
            offset += count;
        }
        return;
    }
    if (index >= g_Papi.rootCapacity) return;
    GpuPapiEmitter emitter = g_Emitters[g_VisibleRoots[index]];
    if (emitter.active != 1 || !GpuRootVisible(emitter)) return;
    GpuCompactEmitter(emitter);
    uint child = emitter.pad0;
    while (child != GPU_PAPI_INVALID) {
        GpuPapiEmitter childEmitter = g_Emitters[child];
        GpuCompactEmitter(childEmitter);
        child = childEmitter.pad0;
    }
}
