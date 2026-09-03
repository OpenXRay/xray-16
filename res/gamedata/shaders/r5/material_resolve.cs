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
Texture2D<float> g_Depth : register(t31);
StructuredBuffer<InstanceData> g_InstanceData : register(t40);
StructuredBuffer<InstanceData> g_TerrainInstanceData : register(t41);
StructuredBuffer<ClusterEntry> g_SkinnedEntries : register(t42);
ByteAddressBuffer g_SkinnedVB : register(t43);
ByteAddressBuffer g_SkinnedIB : register(t44);
ByteAddressBuffer g_SkinnedPrevVB : register(t45);
RWTexture2D<float4> g_OutNormal : register(u0);
RWTexture2D<float4> g_OutBaseColor : register(u1);
RWTexture2D<float4> g_OutColor : register(u2);
RWTexture2D<float2> g_OutMotion : register(u3);
RWTexture2D<float> g_OutVisDepth : register(u4);

cbuffer MaterialResolveParams : register(b5)
{
    float4x4 g_PrevView;
    float4x4 g_PrevProj;
    uint g_SkinnedEntryBase;
    uint g_MotionValid;
    uint2 g_ResolvePad;
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

float4 ProjectPrevEntry(float3 worldPos, bool hud)
{
    float4 viewPos = mul(g_PrevView, float4(worldPos, 1.0));
    if (hud)
        viewPos.xy /= hud_fov;
    return mul(g_PrevProj, viewPos);
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
    SkinnedDrawRecord rec = g_SkinnedRecords[skinned ? e.batchIndex : 0u];
    uint ib = e.ibFirst + tri * 3u;

    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    bool hud = (e.flags & CLUSTER_ENTRY_FLAG_HUD) != 0u;
    float4x4 world = float4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    MegaVertex v0;
    MegaVertex v1;
    MegaVertex v2;
    float3 wp0, wp1, wp2;
    float3 pp0, pp1, pp2;
    if (skinned)
    {
        uint i0 = g_SkinnedIB.Load(ib * 4u);
        uint i1 = g_SkinnedIB.Load((ib + 1u) * 4u);
        uint i2 = g_SkinnedIB.Load((ib + 2u) * 4u);
        v0 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i0);
        v1 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i1);
        v2 = LoadMegaVertex(g_SkinnedVB, e.firstVertex + i2);
        wp0 = v0.position;
        wp1 = v1.position;
        wp2 = v2.position;
        if (rec.prevFirstVertex != 0xFFFFFFFFu)
        {
            pp0 = LoadMegaPosition(g_SkinnedPrevVB, rec.prevFirstVertex + i0);
            pp1 = LoadMegaPosition(g_SkinnedPrevVB, rec.prevFirstVertex + i1);
            pp2 = LoadMegaPosition(g_SkinnedPrevVB, rec.prevFirstVertex + i2);
        }
        else
        {
            pp0 = wp0;
            pp1 = wp1;
            pp2 = wp2;
        }
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
        wp0 = mul(world, float4(v0.position, 1.0)).xyz;
        wp1 = mul(world, float4(v1.position, 1.0)).xyz;
        wp2 = mul(world, float4(v2.position, 1.0)).xyz;
        pp0 = wp0;
        pp1 = wp1;
        pp2 = wp2;
    }
    float3x3 world3 = (float3x3)world;

    float4 c0 = ProjectEntry(wp0, hud);
    float4 c1 = ProjectEntry(wp1, hud);
    float4 c2 = ProjectEntry(wp2, hud);

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
        float3 worldPos = InterpolateBary3(bd, wp0, wp1, wp2);
        s.albedo = mdi_apply_splat_color(rec, s.albedo, worldPos, uv);
        roughnessOut = -max(s.roughness, 0.004);
    }

    float2 motion = float2(0.0, 0.0);
    if (g_MotionValid != 0u)
    {
        float4 prevClip = ProjectPrevEntry(InterpolateBary3(bd, pp0, pp1, pp2), hud);
        if (prevClip.w > 0.0)
        {
            float2 prevNdc = prevClip.xy / prevClip.w;
            motion = float2(prevNdc.x, -prevNdc.y) * 0.5 + 0.5 - uvPix;
        }
    }

    g_OutNormal[p] = float4(s.N, roughnessOut);
    g_OutBaseColor[p] = float4(s.albedo, s.metallic);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, s.ao);
    g_OutMotion[p] = motion;
    g_OutVisDepth[p] = g_Depth.Load(int3(p, 0));
}
