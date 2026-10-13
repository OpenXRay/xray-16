#define SM_6_0
#include "common.h"
#include "local_shadow_common.h"
#define CLUSTER_GEO_T_REFS t14
#define CLUSTER_GEO_T_META t16
#define CLUSTER_GEO_T_INSTANCES t20
#include "cluster_geo_bindings.h"
#include "cluster_geo_payload.h"

StructuredBuffer<uint2> g_Pairs : register(t15);
StructuredBuffer<LocalShadowView> g_LocalShadowTiles : register(t17);

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
    ClusterRefView view = LoadClusterRefView(pair.x);
    ClusterEntry e = view.entry;
    uint slot = pair.y;

    if (vid >= e.indexCount)
    {
        output.position = float4(2.0, 2.0, 2.0, 1.0);
#ifdef TARGET_DXIL
        output.clip = float4(-1.0, -1.0, -1.0, -1.0);
#else
        output.clip[0] = -1.0;
        output.clip[1] = -1.0;
        output.clip[2] = -1.0;
        output.clip[3] = -1.0;
#endif
        output.texcoord = float2(0.0, 0.0);
        output.materialID = 0u;
        return output;
    }

    ClusterGeoView geo = ClusterGeoResolve(e);
    uint triangle = vid / 3u;
    uint corner = vid - triangle * 3u;
    uint3 corners = ClusterPayloadTriangle(geo, triangle);
    uint local = (corner == 0u) ? corners.x : ((corner == 1u) ? corners.y : corners.z);
    uint slotIndex = ClusterPayloadSlot(geo, local);
    float3 position = ClusterLoadPositionSlot(geo, slotIndex);
    float2 texcoord = ClusterLoadUVSlot(geo, slotIndex);

    float4x4 worldMatrix = view.world;
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
