#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "visbuffer_common.h"
#include "detail_blade_common.h"

cbuffer DetailGlobals : register(b3)
{
    float4 consts;
    float4 wave;
    float4 dir2D;
    float4 dir2D_2;
    float4x4 g_detail_VP;
    float4 detail_params;
    float4 g_wind_direction;
    float grass_wind_displacement;
    float grass_interaction_displacement;
    uint interaction_atlas_index;
    uint perlin4d_texture_index;
    float4 grass_color_tip;
    float4 grass_color_base;
    float4 grass_sss_color;
    float grass_color_variation;
    float grass_blade_height;
    uint buildDetailsIndex;
    uint buildDetailsPbrIndex;
    float grass_blade_width;
    float3 detail_pad;
};

cbuffer DetailResolveParams : register(b5)
{
    float4x4 g_PrevView;
    float4x4 g_PrevProj;
    uint g_EntryBase;
    uint g_MotionValid;
    float g_PrevTime;
    uint g_VeinIndex;
    uint4 g_Segments;
};

Texture3D g_Perlin4D : register(t12);
Texture2D<uint> g_VisID : register(t30);
Texture2D<float> g_Depth : register(t31);
StructuredBuffer<uint> g_VisibleLod0 : register(t33);
StructuredBuffer<uint> g_VisibleLod1 : register(t34);
StructuredBuffer<uint> g_VisibleLod2 : register(t35);
StructuredBuffer<GrassObjectTint> grass_object_tints : register(t36);
StructuredBuffer<DetailInstance> all_instances : register(t37);
RWTexture2D<float4> g_OutNormal : register(u0);
RWTexture2D<float4> g_OutBaseColor : register(u1);
RWTexture2D<float4> g_OutColor : register(u2);
RWTexture2D<float2> g_OutMotion : register(u3);
RWTexture2D<float> g_OutVisDepth : register(u4);

static const float GRASS_ROUGHNESS_BASE = 0.85;
static const float GRASS_ROUGHNESS_TIP = 0.55;
static const float GRASS_AO_BASE = 0.35;
static const float GRASS_AO_TIP = 1.0;
static const float GRASS_AO_POWER = 0.6;

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
    if (entryIdx < g_EntryBase)
        return;
    uint rel = entryIdx - g_EntryBase;
    uint lod = rel >> 22;
    uint slot = rel & 0x3FFFFFu;
    uint tri = id & VIS_ID_TRI_MASK;
    if (lod > 2u)
        return;

    uint src = (lod == 0u) ? g_VisibleLod0[slot] : ((lod == 1u) ? g_VisibleLod1[slot] : g_VisibleLod2[slot]);
    uint segments = (lod == 0u) ? g_Segments.x : ((lod == 1u) ? g_Segments.y : g_Segments.z);

    DetailInstance raw = all_instances[src];
    BladeInstance b = DecodeBlade(raw, g_Perlin4D, smp_linear, grass_blade_height);
    BladeWind w = EvalBladeWind(b, wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);

    uint lv0 = BladeTriangleVertex(tri, 0u, segments);
    uint lv1 = BladeTriangleVertex(tri, 1u, segments);
    uint lv2 = BladeTriangleVertex(tri, 2u, segments);
    BladeVertex v0 = EvalBladeVertex(b, w, lv0, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
    BladeVertex v1 = EvalBladeVertex(b, w, lv1, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
    BladeVertex v2 = EvalBladeVertex(b, w, lv2, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);

    float4 c0 = mul(m_VP, float4(v0.pos, 1.0));
    float4 c1 = mul(m_VP, float4(v1.pos, 1.0));
    float4 c2 = mul(m_VP, float4(v2.pos, 1.0));

    float2 uvPix = (float2(p) + 0.5) * screen_res.zw;
    float2 pixelNdc = float2(uvPix.x * 2.0 - 1.0, 1.0 - uvPix.y * 2.0);
    BarycentricDeriv bd = CalcFullBary(c0, c1, c2, pixelNdc, screen_res.xy);

    float2 uv = InterpolateBary2(bd, v0.uv, v1.uv, v2.uv);
    float2 uvDdx = InterpolateBaryDdx2(bd, v0.uv, v1.uv, v2.uv);
    float2 uvDdy = InterpolateBaryDdy2(bd, v0.uv, v1.uv, v2.uv);
    float t = dot(bd.m_lambda, float3(v0.t, v1.t, v2.t));
    float3 worldPos = InterpolateBary3(bd, v0.pos, v1.pos, v2.pos);
    float3 rn1 = InterpolateBary3(bd, v0.rotatedNormal1, v1.rotatedNormal1, v2.rotatedNormal1);
    float3 rn2 = InterpolateBary3(bd, v0.rotatedNormal2, v1.rotatedNormal2, v2.rotatedNormal2);

    float widthPercent = uv.x;
    float3 N = normalize(lerp(rn1, rn2, widthPercent));

    float4 veinDetail = GetBindlessTexture(g_VeinIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
    float veinValue = veinDetail.a;

    float colorBlend = smoothstep(0.2, 0.8, t);
    float3 albedo = lerp(grass_color_base.rgb, grass_color_tip.rgb, colorBlend);
    albedo *= lerp(1.0 - grass_color_variation, 1.0 + grass_color_variation, b.bladeHash);
    GrassObjectTint tint = grass_object_tints[b.objectId];
    albedo *= float3(tint.r, tint.g, tint.b);
    albedo = lerp(albedo, albedo * veinDetail.rgb, veinValue * 0.15);

    float roughness = saturate(lerp(GRASS_ROUGHNESS_BASE, GRASS_ROUGHNESS_TIP, t) + veinValue * 0.1);
    float heightAO = lerp(GRASS_AO_BASE, GRASS_AO_TIP, pow(saturate(t), GRASS_AO_POWER));
    float edgeFactor = abs(widthPercent - 0.5) * 2.0;
    float ao = heightAO * lerp(0.95, 1.0, edgeFactor) * lerp(0.9, 1.0, veinValue);

    float viewDist = length(worldPos - eye_position);
    float distFade = exp(-viewDist * 0.017);
    float3 midColor = (grass_color_base.rgb + grass_color_tip.rgb) * 0.5;
    albedo = lerp(midColor, albedo, distFade);

    float backlit = saturate(dot(N, -L_sun_dir_w) * 0.5 + 0.5);
    float3 sss = grass_sss_color.rgb * (backlit * t * grass_sss_color.w);
    albedo += sss * L_sun_color;

    float2 motion = float2(0.0, 0.0);
    if (g_MotionValid != 0u)
    {
        BladeWind wPrev = EvalBladeWind(b, g_PrevTime, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
        BladeVertex p0 = EvalBladeVertex(b, wPrev, lv0, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        BladeVertex p1 = EvalBladeVertex(b, wPrev, lv1, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        BladeVertex p2 = EvalBladeVertex(b, wPrev, lv2, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        float3 prevWorld = InterpolateBary3(bd, p0.pos, p1.pos, p2.pos);
        float4 prevClip = mul(g_PrevProj, mul(g_PrevView, float4(prevWorld, 1.0)));
        if (prevClip.w > 0.0)
        {
            float2 prevNdc = prevClip.xy / prevClip.w;
            motion = float2(prevNdc.x, -prevNdc.y) * 0.5 + 0.5 - uvPix;
        }
    }

    g_OutNormal[p] = float4(N, roughness);
    g_OutBaseColor[p] = float4(albedo, 0.0);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, ao);
    g_OutMotion[p] = motion;
    g_OutVisDepth[p] = g_Depth.Load(int3(p, 0));
}
