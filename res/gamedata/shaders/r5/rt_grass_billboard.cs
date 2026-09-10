#include "detail_pulled_common.h"

StructuredBuffer<DetailInstance> g_AllInstances : register(t0);
StructuredBuffer<uint> g_VisibleIndices : register(t1);
StructuredBuffer<DetailModelGPU> g_DetailModels : register(t2);
StructuredBuffer<PulledVertex> g_PulledVerts : register(t3);
ByteAddressBuffer g_DrawArgs : register(t4);
RWByteAddressBuffer g_Output : register(u0);
RWByteAddressBuffer g_OutputIB : register(u1);

cbuffer BillboardRTCB : register(b5) {
    uint maxVertsPerBillboard;
    float normalBend;
    uint2 pad;
};

uint pack_normal(float3 n)
{
    uint3 u = uint3(clamp(n * 127.5 + 127.5, 0, 255));
    return (u.x << 16) | (u.y << 8) | u.z;
}

[numthreads(256, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint bb_idx = dtid.x;
    uint actualCount = g_DrawArgs.Load(4);

    uint vertBase = bb_idx * maxVertsPerBillboard;

    if (bb_idx >= actualCount) {
        for (uint v = 0; v < maxVertsPerBillboard; v++) {
            uint vi = vertBase + v;
            g_Output.Store4(vi * 24, uint4(0, 0, 0, 0));
            g_Output.Store2(vi * 24 + 16, uint2(0, 0));
            g_OutputIB.Store(vi * 4, vi);
        }
        return;
    }

    PulledInstance inst = DecodePulled(g_AllInstances[g_VisibleIndices[bb_idx]]);
    DetailModelGPU mdl = g_DetailModels[inst.objectId];

    uint vertCount = min(mdl.pulledIndexCount / 3, maxVertsPerBillboard / 3) * 3;

    for (uint v = 0; v < vertCount; v++) {
        uint vi = vertBase + v;
        PulledVertex pv = g_PulledVerts[mdl.pulledVertexBase + v];
        float3 n = PulledBentNormal(inst, mdl, float3(pv.px, pv.py, pv.pz), normalBend);
        g_Output.Store3(vi * 24, asuint(PulledWorldPos(inst, pv)));
        g_Output.Store(vi * 24 + 12, pack_normal(n));
        g_Output.Store2(vi * 24 + 16, asuint(float2(pv.u, pv.v)));
        g_OutputIB.Store(vi * 4, vi);
    }

    for (uint w = vertCount; w < maxVertsPerBillboard; w++) {
        uint vi = vertBase + w;
        g_Output.Store4(vi * 24, uint4(0, 0, 0, 0));
        g_Output.Store2(vi * 24 + 16, uint2(0, 0));
        g_OutputIB.Store(vi * 4, vi);
    }
}
