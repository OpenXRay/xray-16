#define SM_6_0
#define BINDLESS_NO_IMPLICIT_GRAD
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"
#include "material_eval.h"
#define CLUSTER_GEO_AUTO_BIND
#include "cluster_geo_bindings.h"
#include "cluster_geo_payload.h"

Texture2D<uint> g_VisID;
Texture2D<float> g_Depth;
StructuredBuffer<ClusterEntry> g_SkinnedEntries;
ByteAddressBuffer g_SkinnedVB;
ByteAddressBuffer g_SkinnedIB;
ByteAddressBuffer g_SkinnedPrevVB;
RWTexture2D<float4> g_OutNormal;
RWTexture2D<float4> g_OutBaseColor;
RWTexture2D<float4> g_OutColor;
RWTexture2D<float2> g_OutMotion;
RWTexture2D<float> g_OutVisDepth;
RWTexture2D<float4> g_OutMaterial;

cbuffer MaterialResolveParams
{
    float4x4 g_PrevView;
    float4x4 g_PrevProj;
    float4x4 g_PrevHudWarp;
    uint g_SkinnedEntryBase;
    uint g_MotionValid;
    uint g_EntryLimit;
    uint g_ResolvePad;
};

float4 ProjectEntry(float3 worldPos, bool hud)
{
    float4 p = float4(worldPos, 1.0);
    if (hud)
        p = mul(m_HudWarp, p);
    return mul(m_VP, p);
}

float4 ProjectPrevEntry(float3 worldPos, bool hud)
{
    float4 p = float4(worldPos, 1.0);
    if (hud)
        p = mul(g_PrevHudWarp, p);
    return mul(g_PrevProj, mul(g_PrevView, p));
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
    if (!VisIDValid(id))
        return;

    uint entryIdx = UnpackVisEntry(id);
    if (entryIdx >= g_EntryLimit)
        return;
    uint tri = UnpackVisTri(id);
    bool skinned = entryIdx >= g_SkinnedEntryBase;
    ClusterRefView view = (ClusterRefView)0;
    if (!skinned)
        view = LoadClusterRefView(entryIdx);
    ClusterEntry e = skinned ? g_SkinnedEntries[entryIdx - g_SkinnedEntryBase] : view.entry;
    SkinnedDrawRecord rec = g_SkinnedRecords[skinned ? e.batchIndex : 0u];

    bool terrain = (e.flags & CLUSTER_ENTRY_FLAG_TERRAIN) != 0u;
    bool hud = (e.flags & CLUSTER_ENTRY_FLAG_HUD) != 0u;
    bool dynamic = (e.flags & CLUSTER_ENTRY_FLAG_DYNAMIC) != 0u;
    float4x4 world = float4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    MegaVertex v0;
    MegaVertex v1;
    MegaVertex v2;
    float3 wp0, wp1, wp2;
    float3 pp0, pp1, pp2;
    float3 hemiCorners = float3(1.0, 1.0, 1.0);
    float2 lightmapCorners[3] = { float2(0.0, 0.0), float2(0.0, 0.0), float2(0.0, 0.0) };
    bool bakedHemi = false;
    uint lightmapTexture = INVALID_TEXTURE_INDEX;
    if (skinned)
    {
        uint ib = e.ibFirst + tri * 3u;
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
        ClusterGeoView geo = ClusterGeoResolve(e);
        if (tri >= geo.triangleCount)
            return;
        uint3 corners = ClusterPayloadTriangle(geo, tri);
        uint3 slots = uint3(ClusterPayloadSlot(geo, corners.x), ClusterPayloadSlot(geo, corners.y), ClusterPayloadSlot(geo, corners.z));
        ClusterVertex c0 = ClusterLoadVertexSlot(geo, slots.x);
        ClusterVertex c1 = ClusterLoadVertexSlot(geo, slots.y);
        ClusterVertex c2 = ClusterLoadVertexSlot(geo, slots.z);
        hemiCorners = float3(c0.hemi, c1.hemi, c2.hemi);
        bakedHemi = geo.vertexFormat == CLUSTER_VERTEX_PACKED_BASIS;
        if (view.lightmapTexture != INVALID_TEXTURE_INDEX && (geo.attributeMask & CLUSTER_PAGE_ATTR_UV1) != 0u)
        {
            lightmapTexture = view.lightmapTexture;
            lightmapCorners[0] = ClusterLoadUV1Slot(geo, slots.x);
            lightmapCorners[1] = ClusterLoadUV1Slot(geo, slots.y);
            lightmapCorners[2] = ClusterLoadUV1Slot(geo, slots.z);
        }
        v0.position = c0.position; v0.normal = c0.normal; v0.tangent = c0.tangent; v0.binormal = c0.binormal; v0.uv = c0.uv;
        v1.position = c1.position; v1.normal = c1.normal; v1.tangent = c1.tangent; v1.binormal = c1.binormal; v1.uv = c1.uv;
        v2.position = c2.position; v2.normal = c2.normal; v2.tangent = c2.tangent; v2.binormal = c2.binormal; v2.uv = c2.uv;
        world = view.world;
        wp0 = mul(world, float4(v0.position, 1.0)).xyz;
        wp1 = mul(world, float4(v1.position, 1.0)).xyz;
        wp2 = mul(world, float4(v2.position, 1.0)).xyz;
        if (dynamic && view.historyValid != 0u)
        {
            float4x4 prevWorld = view.prevWorld;
            pp0 = mul(prevWorld, float4(v0.position, 1.0)).xyz;
            pp1 = mul(prevWorld, float4(v1.position, 1.0)).xyz;
            pp2 = mul(prevWorld, float4(v2.position, 1.0)).xyz;
        }
        else
        {
            pp0 = wp0;
            pp1 = wp1;
            pp2 = wp2;
        }
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
    if (s.shadingClass == SHADING_CLASS_FOLIAGE)
        s.N = FoliageViewerNormal(s.N, wp1 - wp0, wp2 - wp0, eye_position - InterpolateBary3(bd, wp0, wp1, wp2));

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
    g_OutColor[p] = float4(s.emissive, s.ao);
    float3 geometricNormal = FaceToward(normalize(cross(wp1 - wp0, wp2 - wp0)), eye_position - InterpolateBary3(bd, wp0, wp1, wp2));
    if (!dynamic && !hud && sky_ibl.z < 0.5 && (lightmapTexture != INVALID_TEXTURE_INDEX || bakedHemi))
    {
        float skyVisibility;
        if (lightmapTexture != INVALID_TEXTURE_INDEX)
        {
            float2 lightmapUV = InterpolateBary2(bd, lightmapCorners[0], lightmapCorners[1], lightmapCorners[2]);
            skyVisibility = GetBindlessTexture(lightmapTexture).SampleLevel(smp_rtlinear, lightmapUV, 0).a;
        }
        else
            skyVisibility = dot(bd.m_lambda, hemiCorners) * view.hemiScale + view.hemiBias;
        g_OutMaterial[p] = PackGBufferMaterialBakedSky(s.shadingClass, s.transmission, skyVisibility);
    }
    else
        g_OutMaterial[p] = PackGBufferMaterial(s.shadingClass, s.transmission, geometricNormal);
    g_OutMotion[p] = motion;
    g_OutVisDepth[p] = g_Depth.Load(int3(p, 0));
}
