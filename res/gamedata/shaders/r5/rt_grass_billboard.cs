#include "detail_pulled_common.h"
#include "detail_source_common.h"
#include "sw_dispatch_common.h"

StructuredBuffer<uint2> g_VisibleIndices : register(t1);
StructuredBuffer<DetailModelGPU> g_DetailModels : register(t2);
StructuredBuffer<PulledVertex> g_PulledVerts : register(t3);
RWByteAddressBuffer g_Output : register(u0);
RWByteAddressBuffer g_OutputIB : register(u1);

cbuffer BillboardRTCB : register(b5) {
    uint maxVertsPerBillboard;
    uint billboardCount;
    uint outputVertexOffset;
    uint outputIndexOffset;
};

uint pack_normal(float3 n)
{
    uint3 u = uint3(clamp(n * 127.5 + 127.5, 0, 255));
    return (u.x << 16) | (u.y << 8) | u.z;
}

[numthreads(256, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint bb_idx = SwDispatchLinearGroup(group_id) * 256u + thread_id.x;
    if (bb_idx >= billboardCount)
        return;

    uint vertBase = bb_idx * maxVertsPerBillboard;


    PulledInstance inst = DecodePulled(LoadDetailInstance(g_VisibleIndices[bb_idx]));
    DetailModelGPU mdl = g_DetailModels[inst.objectId];

    uint vertCount = min(mdl.pulledIndexCount / 3, maxVertsPerBillboard / 3) * 3;

    for (uint t = 0; t < vertCount; t += 3) {
        PulledVertex pv0 = g_PulledVerts[mdl.pulledVertexBase + t];
        PulledVertex pv1 = g_PulledVerts[mdl.pulledVertexBase + t + 1];
        PulledVertex pv2 = g_PulledVerts[mdl.pulledVertexBase + t + 2];
        float3 p0 = PulledWorldPos(inst, pv0);
        float3 p1 = PulledWorldPos(inst, pv1);
        float3 p2 = PulledWorldPos(inst, pv2);
        uint packedN = pack_normal(PulledFaceNormal(p0, p1, p2));
        for (uint c = 0; c < 3; c++) {
            uint local = vertBase + t + c;
            uint vi = outputVertexOffset + local;
            PulledVertex pv = (c == 0) ? pv0 : ((c == 1) ? pv1 : pv2);
            g_Output.Store3(vi * 24, asuint((c == 0) ? p0 : ((c == 1) ? p1 : p2)));
            g_Output.Store(vi * 24 + 12, packedN);
            g_Output.Store2(vi * 24 + 16, asuint(float2(pv.u, pv.v)));
            g_OutputIB.Store((outputIndexOffset + local) * 4, vi);
        }
    }

    for (uint w = vertCount; w < maxVertsPerBillboard; w++) {
        uint local = vertBase + w;
        uint vi = outputVertexOffset + local;
        g_Output.Store4(vi * 24, uint4(0, 0, 0, 0));
        g_Output.Store2(vi * 24 + 16, uint2(0, 0));
        g_OutputIB.Store((outputIndexOffset + local) * 4, vi);
    }
}
