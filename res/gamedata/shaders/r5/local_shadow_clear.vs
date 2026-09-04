#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"

StructuredBuffer<LocalShadowTile> g_LocalShadowTiles : register(t17);
StructuredBuffer<uint> g_Refresh : register(t16);

struct VS_OUTPUT
{
    float4 position : SV_Position;
};

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    uint slot = g_Refresh[instanceID];
    float4 rect = g_LocalShadowTiles[slot].rect;
    float2 corner = (vertexID == 0u) ? float2(0.0, 0.0) :
                    (vertexID == 1u) ? float2(1.0, 0.0) :
                    (vertexID == 2u) ? float2(1.0, 1.0) :
                    (vertexID == 3u) ? float2(0.0, 0.0) :
                    (vertexID == 4u) ? float2(1.0, 1.0) : float2(0.0, 1.0);
    float2 uv = (rect.xy + corner * rect.z) / LOCAL_SHADOW_ATLAS;
    VS_OUTPUT o;
    o.position = float4(uv.x * 2.0 - 1.0, -(uv.y * 2.0 - 1.0), 0.0, 1.0);
    return o;
}
