#define SM_5_0
#include "common.h"
#include "vsm_common.h"

StructuredBuffer<uint> g_DirtyList : register(t0);

struct VS_OUTPUT
{
    float4 position : SV_Position;
};

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    uint slot = g_DirtyList[instanceID];
    uint ax = slot % uint(VSM_ATLAS_W_S);
    uint ay = slot / uint(VSM_ATLAS_W_S);
    float2 corner = (vertexID == 0u) ? float2(0.0, 0.0) :
                    (vertexID == 1u) ? float2(1.0, 0.0) :
                    (vertexID == 2u) ? float2(1.0, 1.0) :
                    (vertexID == 3u) ? float2(0.0, 0.0) :
                    (vertexID == 4u) ? float2(1.0, 1.0) : float2(0.0, 1.0);
    float2 cellMin = float2(float(ax), float(ay)) / float2(float(VSM_ATLAS_W_S), float(VSM_ATLAS_H_S));
    float2 cellSize = 1.0 / float2(float(VSM_ATLAS_W_S), float(VSM_ATLAS_H_S));
    float2 uv = cellMin + corner * cellSize;
    VS_OUTPUT o;
    o.position = float4(uv.x * 2.0 - 1.0, -(uv.y * 2.0 - 1.0), 0.0, 1.0);
    return o;
}
