#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "material_eval.h"
#include "visbuffer_common.h"

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<InstanceData> g_TerrainInstanceData : register(t15);
StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);
Texture2D<uint> g_VisID : register(t30);
RWTexture2D<float4> g_OutNormal : register(u0);
RWTexture2D<float4> g_OutBaseColor : register(u1);
RWTexture2D<float4> g_OutColor : register(u2);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint width, height;
    g_VisID.GetDimensions(width, height);
    uint2 p = dtid.xy;
    if (p.x >= width || p.y >= height)
        return;

    uint id = g_VisID[p];
    if (id == 0u)
        return;

    ClusterEntry e = g_Entries[id >> VIS_ID_TRI_BITS];
    uint tri = id & VIS_ID_TRI_MASK;
    uint ib = e.ibFirst + tri * 3u;
    uint i0 = g_MegaIB.Load(ib * 4u);
    uint i1 = g_MegaIB.Load((ib + 1u) * 4u);
    uint i2 = g_MegaIB.Load((ib + 2u) * 4u);

    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    float4x4 world = terrain ? g_TerrainInstanceData[e.batchIndex].world : g_InstanceData[e.batchIndex].world;
    float3x3 world3 = (float3x3)world;

    MegaVertex v0 = LoadMegaVertex(g_MegaVB, e.firstVertex + i0);
    MegaVertex v1 = LoadMegaVertex(g_MegaVB, e.firstVertex + i1);
    MegaVertex v2 = LoadMegaVertex(g_MegaVB, e.firstVertex + i2);

    float4 c0 = mul(m_VP, mul(world, float4(v0.position, 1.0)));
    float4 c1 = mul(m_VP, mul(world, float4(v1.position, 1.0)));
    float4 c2 = mul(m_VP, mul(world, float4(v2.position, 1.0)));

    float2 uvPix = (float2(p) + 0.5) * screen_res.zw;
    float2 pixelNdc = float2(uvPix.x * 2.0 - 1.0, 1.0 - uvPix.y * 2.0);
    BarycentricDeriv bd = CalcFullBary(c0, c1, c2, pixelNdc, screen_res.xy);

    float2 uv = InterpolateBary2(bd, v0.uv, v1.uv, v2.uv);
    float2 uvDdx = InterpolateBaryDdx2(bd, v0.uv, v1.uv, v2.uv);
    float2 uvDdy = InterpolateBaryDdy2(bd, v0.uv, v1.uv, v2.uv);

    float3 n = InterpolateBary3(bd, normalize(mul(world3, v0.normal)), normalize(mul(world3, v1.normal)), normalize(mul(world3, v2.normal)));
    float3 t = InterpolateBary3(bd, normalize(mul(world3, v0.tangent)), normalize(mul(world3, v1.tangent)), normalize(mul(world3, v2.tangent)));
    float3 b = InterpolateBary3(bd, normalize(mul(world3, v0.binormal)), normalize(mul(world3, v1.binormal)), normalize(mul(world3, v2.binormal)));

    MaterialSurface s;
    if (terrain)
    {
        s = EvalTerrainMaterial(g_TerrainMaterials[e.materialID], uv, uvDdx, uvDdy, n, t, b);
    }
    else
    {
        MaterialData mat = g_Materials[e.materialID];
        float4 diffuse = SampleDiffuseGrad(mat, uv, uvDdx, uvDdy);
        s = EvalStandardMaterial(mat, diffuse.rgb, uv, uvDdx, uvDdy, n, t, b);
    }

    g_OutNormal[p] = float4(s.N, s.roughness);
    g_OutBaseColor[p] = float4(s.albedo, s.metallic);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, s.ao);
}
