#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "visbuffer_common.h"
#include "detail_blade_common.h"
#include "detail_pulled_common.h"

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
    float grass_interaction_max_angle;
    float grass_blade_width;
    float4 grass_color_tip;
    float4 grass_color_base;
    float grass_color_variation;
    float grass_blade_height;
    uint buildDetailsIndex;
    uint buildDetailsPbrIndex;
    float4 interaction_window;
    float4 interaction_window_prev;
    float grass_normal_bend;
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
    uint g_InteractionDebug;
    uint g_ResolvePad0;
    uint g_ResolvePad1;
    uint g_ResolvePad2;
};

Texture3D g_Perlin4D : register(t12);
Texture2D g_Interaction : register(t13);
Texture2D g_InteractionPrev : register(t14);
Texture2D<uint> g_VisID : register(t30);
Texture2D<float> g_Depth : register(t31);
StructuredBuffer<uint> g_VisibleLod0 : register(t33);
StructuredBuffer<uint> g_VisibleLod1 : register(t34);
StructuredBuffer<uint> g_VisibleLod2 : register(t35);
StructuredBuffer<GrassObjectTint> grass_object_tints : register(t36);
StructuredBuffer<DetailInstance> all_instances : register(t37);
StructuredBuffer<uint> g_VisibleMesh : register(t38);
StructuredBuffer<uint> g_VisibleDecal : register(t39);
StructuredBuffer<DetailModelGPU> detail_models : register(t40);
StructuredBuffer<PulledVertex> pulled_vertices : register(t41);
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

float2 PrevMotion(float3 prevWorld, float2 uvPix)
{
    float4 prevClip = mul(g_PrevProj, mul(g_PrevView, float4(prevWorld, 1.0)));
    if (prevClip.w <= 0.0)
        return float2(0.0, 0.0);
    float2 prevNdc = prevClip.xy / prevClip.w;
    return float2(prevNdc.x, -prevNdc.y) * 0.5 + 0.5 - uvPix;
}

void ResolvePulled(uint2 p, uint kind, uint slot, uint tri, float2 uvPix, float2 pixelNdc)
{
    uint src = (kind == DETAIL_KIND_MESH) ? g_VisibleMesh[slot] : g_VisibleDecal[slot];
    DetailInstance raw = all_instances[src];
    PulledInstance inst = DecodePulled(raw);
    DetailModelGPU mdl = detail_models[inst.objectId];
    uint base = mdl.pulledVertexBase + tri * 3u;
    PulledVertex pv0 = pulled_vertices[base];
    PulledVertex pv1 = pulled_vertices[base + 1u];
    PulledVertex pv2 = pulled_vertices[base + 2u];
    bool sway = (kind == DETAIL_KIND_MESH);

    float3 h = float3(PulledHeightFactor(pv0, mdl), PulledHeightFactor(pv1, mdl), PulledHeightFactor(pv2, mdl));
    float3 b0 = PulledWorldPos(inst, pv0);
    float3 b1 = PulledWorldPos(inst, pv1);
    float3 b2 = PulledWorldPos(inst, pv2);
    float3 w0 = b0;
    float3 w1 = b1;
    float3 w2 = b2;
    float2 inter = float2(0.0, 0.0);
    if (sway)
    {
        inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, inst.pos.xz, interaction_window);
        w0 = PulledInteractionBend(inst, b0, inter, grass_interaction_displacement, grass_interaction_max_angle);
        w1 = PulledInteractionBend(inst, b1, inter, grass_interaction_displacement, grass_interaction_max_angle);
        w2 = PulledInteractionBend(inst, b2, inter, grass_interaction_displacement, grass_interaction_max_angle);
        w0 = PulledSway(w0, h.x, wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
        w1 = PulledSway(w1, h.y, wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
        w2 = PulledSway(w2, h.z, wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
    }

    float4 c0 = mul(m_VP, float4(w0, 1.0));
    float4 c1 = mul(m_VP, float4(w1, 1.0));
    float4 c2 = mul(m_VP, float4(w2, 1.0));
    BarycentricDeriv bd = CalcFullBary(c0, c1, c2, pixelNdc, screen_res.xy);

    float2 uv0 = float2(pv0.u, pv0.v);
    float2 uv1 = float2(pv1.u, pv1.v);
    float2 uv2 = float2(pv2.u, pv2.v);
    float2 uv = InterpolateBary2(bd, uv0, uv1, uv2);
    float2 uvDdx = InterpolateBaryDdx2(bd, uv0, uv1, uv2);
    float2 uvDdy = InterpolateBaryDdy2(bd, uv0, uv1, uv2);
    float heightParam = dot(bd.m_lambda, h);
    float3 local = InterpolateBary3(bd, float3(pv0.px, pv0.py, pv0.pz), float3(pv1.px, pv1.py, pv1.pz), float3(pv2.px, pv2.py, pv2.pz));
    float3 N = sway ? PulledBentNormal(inst, mdl, local, grass_normal_bend) : float3(0.0, 1.0, 0.0);

    float4 texel = GetBindlessTexture(buildDetailsIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
    float3 albedo = texel.rgb;
    float metallic = 0.0;
    float roughness = 0.85;
    float ao = sway ? lerp(0.5, 1.0, saturate(heightParam)) : 1.0;
    if (buildDetailsPbrIndex != 0u)
    {
        float4 pbr = GetBindlessTexture(buildDetailsPbrIndex).SampleGrad(smp_linear, uv, uvDdx, uvDdy);
        metallic = pbr.r;
        roughness = pbr.g;
        ao = pbr.b;
    }
    if (sway && g_InteractionDebug != 0u)
        albedo = lerp(albedo, float3(1.0, 0.0, 0.0), saturate(length(inter) * 4.0));

    float2 motion = float2(0.0, 0.0);
    if (g_MotionValid != 0u)
    {
        float3 p0 = b0;
        float3 p1 = b1;
        float3 p2 = b2;
        if (sway)
        {
            float2 interPrev = SampleGrassInteraction(g_InteractionPrev, smp_rtlinear, inst.pos.xz, interaction_window_prev);
            p0 = PulledInteractionBend(inst, b0, interPrev, grass_interaction_displacement, grass_interaction_max_angle);
            p1 = PulledInteractionBend(inst, b1, interPrev, grass_interaction_displacement, grass_interaction_max_angle);
            p2 = PulledInteractionBend(inst, b2, interPrev, grass_interaction_displacement, grass_interaction_max_angle);
            p0 = PulledSway(p0, h.x, g_PrevTime, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
            p1 = PulledSway(p1, h.y, g_PrevTime, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
            p2 = PulledSway(p2, h.z, g_PrevTime, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
        }
        motion = PrevMotion(InterpolateBary3(bd, p0, p1, p2), uvPix);
    }

    g_OutNormal[p] = float4(N, roughness);
    g_OutBaseColor[p] = float4(albedo, metallic);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, -max(ao, 0.004));
    g_OutMotion[p] = motion;
    g_OutVisDepth[p] = g_Depth.Load(int3(p, 0));
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
    if (entryIdx < g_EntryBase)
        return;
    uint rel = entryIdx - g_EntryBase;
    uint kind = rel >> 22;
    uint slot = rel & 0x3FFFFFu;
    uint tri = id & VIS_ID_TRI_MASK;
    if (kind > DETAIL_KIND_DECAL)
        return;

    float2 uvPix = (float2(p) + 0.5) * screen_res.zw;
    float2 pixelNdc = float2(uvPix.x * 2.0 - 1.0, 1.0 - uvPix.y * 2.0);

    if (kind >= DETAIL_KIND_MESH)
    {
        ResolvePulled(p, kind, slot, tri, uvPix, pixelNdc);
        return;
    }

    uint lod = kind;
    uint src = (lod == 0u) ? g_VisibleLod0[slot] : ((lod == 1u) ? g_VisibleLod1[slot] : g_VisibleLod2[slot]);
    uint segments = (lod == 0u) ? g_Segments.x : ((lod == 1u) ? g_Segments.y : g_Segments.z);

    DetailInstance raw = all_instances[src];
    BladeInstance b = DecodeBlade(raw, g_Perlin4D, smp_linear, grass_blade_height);
    float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, b.pos.xz, interaction_window);
    BladeBend w = EvalBladeBend(b, wave.w, g_wind_direction.xy, grass_wind_displacement, inter, grass_interaction_displacement, grass_interaction_max_angle, g_Perlin4D, smp_linear);

    uint lv0 = BladeTriangleVertex(tri, 0u, segments);
    uint lv1 = BladeTriangleVertex(tri, 1u, segments);
    uint lv2 = BladeTriangleVertex(tri, 2u, segments);
    BladeVertex v0 = EvalBladeVertex(b, w, lv0, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
    BladeVertex v1 = EvalBladeVertex(b, w, lv1, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
    BladeVertex v2 = EvalBladeVertex(b, w, lv2, segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);

    float4 c0 = mul(m_VP, float4(v0.pos, 1.0));
    float4 c1 = mul(m_VP, float4(v1.pos, 1.0));
    float4 c2 = mul(m_VP, float4(v2.pos, 1.0));
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

    if (g_InteractionDebug != 0u)
        albedo = lerp(albedo, float3(1.0, 0.0, 0.0), saturate(length(inter) * 4.0));

    float2 motion = float2(0.0, 0.0);
    if (g_MotionValid != 0u)
    {
        float2 interPrev = SampleGrassInteraction(g_InteractionPrev, smp_rtlinear, b.pos.xz, interaction_window_prev);
        BladeBend wPrev = EvalBladeBend(b, g_PrevTime, g_wind_direction.xy, grass_wind_displacement, interPrev, grass_interaction_displacement, grass_interaction_max_angle, g_Perlin4D, smp_linear);
        BladeVertex p0 = EvalBladeVertex(b, wPrev, lv0, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        BladeVertex p1 = EvalBladeVertex(b, wPrev, lv1, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        BladeVertex p2 = EvalBladeVertex(b, wPrev, lv2, segments, g_PrevTime, grass_blade_width, g_Perlin4D, smp_linear);
        motion = PrevMotion(InterpolateBary3(bd, p0.pos, p1.pos, p2.pos), uvPix);
    }

    g_OutNormal[p] = float4(N, roughness);
    g_OutBaseColor[p] = float4(albedo, 0.0);
    g_OutColor[p] = float4(0.0, 0.0, 0.0, -max(ao, 0.004));
    g_OutMotion[p] = motion;
    g_OutVisDepth[p] = g_Depth.Load(int3(p, 0));
}
