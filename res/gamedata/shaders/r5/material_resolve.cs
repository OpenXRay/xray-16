#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"
#include "material_eval.h"
#include "visbuffer_common.h"

StructuredBuffer<ClusterEntry> g_Entries : register(t16);
ByteAddressBuffer g_MegaVB : register(t18);
ByteAddressBuffer g_MegaIB : register(t19);
Texture2D<uint> g_VisID : register(t30);
StructuredBuffer<InstanceData> g_InstanceData : register(t40);
StructuredBuffer<InstanceData> g_TerrainInstanceData : register(t41);
StructuredBuffer<ClusterEntry> g_SkinnedEntries : register(t42);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);
RWTexture2D<float4> g_OutNormal : register(u0);
RWTexture2D<float4> g_OutBaseColor : register(u1);
RWTexture2D<float4> g_OutColor : register(u2);

cbuffer MaterialResolveParams : register(b5)
{
    uint g_SkinnedEntryBase;
    uint3 g_ResolvePad;
};

float4 ProjectEntry(float3 worldPos, bool hud)
{
    if (hud)
    {
        float4 viewPos = mul(m_V, float4(worldPos, 1.0));
        viewPos.xy /= hud_fov;
        return mul(m_P, viewPos);
    }
    return mul(m_VP, float4(worldPos, 1.0));
}

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

    uint entryIdx = id >> VIS_ID_TRI_BITS;
    uint tri = id & VIS_ID_TRI_MASK;
    bool skinned = entryIdx >= g_SkinnedEntryBase;
    ClusterEntry e = skinned ? g_SkinnedEntries[entryIdx - g_SkinnedEntryBase] : g_Entries[entryIdx];
    uint ib = e.ibFirst + tri * 3u;

    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    bool hud = (e.flags & CLUSTER_ENTRY_FLAG_HUD) != 0u;
    float4x4 world = float4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    MegaVertex v0;
    MegaVertex v1;
    MegaVertex v2;
    if (skinned)
    {
        uint i0 = g_SkinnedIB.Load(ib * 4u);
        uint i1 = g_SkinnedIB.Load((ib + 1u) * 4u);
        uint i2 = g_SkinnedIB.Load((ib + 2u) * 4u);
        v0 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i0);
        v1 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i1);
        v2 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i2);
    }
    else
    {
        uint i0 = g_MegaIB.Load(ib * 4u);
        uint i1 = g_MegaIB.Load((ib + 1u) * 4u);
        uint i2 = g_MegaIB.Load((ib + 2u) * 4u);
        v0 = LoadMegaVertex(g_MegaVB, e.firstVertex + i0);
        v1 = LoadMegaVertex(g_MegaVB, e.firstVertex + i1);
        v2 = LoadMegaVertex(g_MegaVB, e.firstVertex + i2);
        world = terrain ? g_TerrainInstanceData[e.batchIndex].world : g_InstanceData[e.batchIndex].world;
    }
    float3x3 world3 = (float3x3)world;

    float4 c0 = ProjectEntry(mul(world, float4(v0.position, 1.0)).xyz, hud);
    float4 c1 = ProjectEntry(mul(world, float4(v1.position, 1.0)).xyz, hud);
    float4 c2 = ProjectEntry(mul(world, float4(v2.position, 1.0)).xyz, hud);

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

    float roughnessOut = s.roughness;
    if (skinned)
    {
        SkinnedDrawRecord rec = g_SkinnedRecords[e.batchIndex];
        float3 worldPos = InterpolateBary3(bd, v0.position, v1.position, v2.position);
        s.albedo = mdi_apply_splat_color(rec, s.albedo, worldPos, uv);
        roughnessOut = -max(s.roughness, 0.004);
    }

    g_OutNormal[p] = float4(s.N, roughnessOut);
    g_OutBaseColor[p] = float4(s.albedo, s.metallic);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, s.ao);
}
