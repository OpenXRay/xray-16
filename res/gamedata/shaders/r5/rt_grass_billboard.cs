struct InstanceData {
    float3 pos;
    uint packed;
};

struct DetailModelGPU {
    float minScale;
    float maxScale;
    float flags;
    float geomExtentX;
    float geomExtentZ;
    float uv_min_x;
    float uv_min_y;
    float uv_max_x;
    float uv_max_y;
    uint pulledVertexBase;
    uint pulledIndexCount;
    float geomExtentY;
};

struct PulledVertex {
    float px, py, pz;
    float u, v;
};

StructuredBuffer<InstanceData> g_AllInstances : register(t0);
StructuredBuffer<uint> g_VisibleIndices : register(t1);
StructuredBuffer<DetailModelGPU> g_DetailModels : register(t2);
StructuredBuffer<PulledVertex> g_PulledVerts : register(t3);
ByteAddressBuffer g_DrawArgs : register(t4);
RWByteAddressBuffer g_Output : register(u0);
RWByteAddressBuffer g_OutputIB : register(u1);

cbuffer BillboardRTCB : register(b5) {
    uint maxVertsPerBillboard;
    uint3 pad;
};

static const float TWO_PI = 6.28318530718;
static const float PACK_MAX_SCALE = 4.0;

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

    uint src_idx = g_VisibleIndices[bb_idx];
    InstanceData inst = g_AllInstances[src_idx];

    uint object_id = inst.packed & 0x3F;
    float rotation = float((inst.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
    float scale = float((inst.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;

    DetailModelGPU mdl = g_DetailModels[object_id];
    float c = cos(rotation), s = sin(rotation);

    uint triCount = min(mdl.pulledIndexCount / 3, maxVertsPerBillboard / 3);

    for (uint tri = 0; tri < triCount; tri++) {
        float3 positions[3];
        float2 uvs[3];

        [unroll]
        for (uint k = 0; k < 3; k++) {
            PulledVertex pv = g_PulledVerts[mdl.pulledVertexBase + tri * 3 + k];
            float3 lp = float3(pv.px, pv.py, pv.pz) * scale;
            positions[k] = float3(lp.x * c - lp.z * s, lp.y, lp.x * s + lp.z * c) + inst.pos;
            uvs[k] = float2(pv.u, pv.v);
        }

        float3 normal = cross(positions[1] - positions[0], positions[2] - positions[0]);
        float nlen = length(normal);
        normal = (nlen > 0.001) ? (normal / nlen) : float3(0, 1, 0);
        uint packed_n = pack_normal(normal);

        [unroll]
        for (uint k = 0; k < 3; k++) {
            uint vi = vertBase + tri * 3 + k;
            g_Output.Store3(vi * 24, asuint(positions[k]));
            g_Output.Store(vi * 24 + 12, packed_n);
            g_Output.Store2(vi * 24 + 16, asuint(uvs[k]));
            g_OutputIB.Store(vi * 4, vi);
        }
    }

    for (uint v = triCount * 3; v < maxVertsPerBillboard; v++) {
        uint vi = vertBase + v;
        g_Output.Store4(vi * 24, uint4(0, 0, 0, 0));
        g_Output.Store2(vi * 24 + 16, uint2(0, 0));
        g_OutputIB.Store(vi * 4, vi);
    }
}
