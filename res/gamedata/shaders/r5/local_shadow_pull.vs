#define SM_6_0
#include "common.h"
#include "local_shadow_common.h"
#include "visbuffer_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint2> g_Pairs : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
StructuredBuffer<LocalShadowView> g_LocalShadowTiles : register(t17);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);

#include "local_shadow_route.h"

struct VS_OUTPUT
{
    precise float4 position : SV_Position;
#ifdef TARGET_DXIL
    float4 clip : SV_ClipDistance;
#else
    float clip[4] : SV_ClipDistance;
#endif
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    uint2 pair = g_Pairs[iid];
    ClusterEntry e = g_Entries[pair.x];
    uint slot = pair.y;

    uint local = min(vid, e.indexCount - 1u);
    uint index = g_MegaIB.Load((e.ibFirst + local) * 4u);
    uint vertexByte = (e.firstVertex + index) * 48u;
    uint4 w0 = g_MegaVB.Load4(vertexByte);
    uint4 w1 = g_MegaVB.Load4(vertexByte + 16u);
    float3 position = float3(asfloat(w0.x), asfloat(w0.y), asfloat(w0.z));
    float2 texcoord = float2(asfloat(w1.z), asfloat(w1.w));

    float4x4 worldMatrix = g_InstanceData[e.batchIndex].world;
    float3 worldPos = mul(worldMatrix, float4(position, 1.0)).xyz;

    LocalRoute r = LocalRouteTile(slot, worldPos);
    output.position = r.position;
#ifdef TARGET_DXIL
    output.clip = r.clip;
#else
    output.clip[0] = r.clip.x;
    output.clip[1] = r.clip.y;
    output.clip[2] = r.clip.z;
    output.clip[3] = r.clip.w;
#endif
    output.texcoord = texcoord;
    output.materialID = e.materialID;
    return output;
}
