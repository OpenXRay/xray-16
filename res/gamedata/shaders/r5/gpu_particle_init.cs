#include "gpu_particle_types.h"
cbuffer GpuPapiParams { GpuPapiConstants g_Papi; }
RWStructuredBuffer<GpuPapiEmitter> g_Emitters;
RWStructuredBuffer<uint> g_ParticleAllocation;
RWStructuredBuffer<uint> g_StateAllocation;
RWStructuredBuffer<uint> g_Errors;
[numthreads(256,1,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
    uint index = dispatchID.x;
    if (g_Papi.phase == 1) {
        if (index == 0) InterlockedAnd(g_Errors[0],~7u);
        return;
    }
    if (index < g_Papi.emitterCapacity) g_Emitters[index] = (GpuPapiEmitter)0;
    if (index < g_Papi.particleCapacity) g_ParticleAllocation[index] = 0;
    if (index < g_Papi.actionStateCapacity) g_StateAllocation[index] = 0;
    if (index == 0) g_Errors[0] = 0;
}
