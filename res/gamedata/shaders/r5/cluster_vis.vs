#define SM_6_0
#include "shared/common.h"
#define CLUSTER_GEO_T_REFS t14
#define CLUSTER_GEO_T_META t16
#define CLUSTER_GEO_T_INSTANCES t20
#include "cluster_geo_bindings.h"
#include "cluster_geo_payload.h"

StructuredBuffer<uint> g_VisibleEntries : register(t15);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
    nointerpolation uint drawID : TEXCOORD2;
    nointerpolation uint visID : TEXCOORD3;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VS_OUTPUT output;

    uint slot = iid;
    uint refIdx = g_VisibleEntries[slot];
    ClusterRefView view = LoadClusterRefView(refIdx);
    ClusterEntry e = view.entry;

    if (vid >= e.indexCount)
    {
        output.position = float4(2.0, 2.0, 2.0, 1.0);
        output.texcoord = float2(0.0, 0.0);
        output.materialID = 0u;
        output.drawID = 0u;
        output.visID = VIS_ID_BACKGROUND;
        return output;
    }

    ClusterGeoView geo = ClusterGeoResolve(e);
    uint triangle = vid / 3u;
    uint corner = vid - triangle * 3u;
    uint3 corners = ClusterPayloadTriangle(geo, triangle);
    uint local = (corner == 0u) ? corners.x : ((corner == 1u) ? corners.y : corners.z);
    uint slotIndex = ClusterPayloadSlot(geo, local);

    float3 position = ClusterLoadPositionSlot(geo, slotIndex);
    float4 worldPos = mul(view.world, float4(position, 1.0));
    output.position = mul(m_VP, float4(worldPos.xyz, 1.0));
    output.texcoord = ClusterLoadUVSlot(geo, slotIndex);
    output.materialID = e.materialID;
    output.drawID = slot;
    output.visID = PackVisID(refIdx, triangle);
    return output;
}
