#define SM_6_0
#include "common.h"
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
    float4 grass_sss_color;
    float grass_color_variation;
    float grass_blade_height;
    uint buildDetailsIndex;
    uint buildDetailsPbrIndex;
    float4 interaction_window;
    float4 interaction_window_prev;
};

cbuffer DetailVisParams : register(b5)
{
    uint g_EntryBase;
    uint g_Kind;
    uint g_Segments;
    float g_AlphaRef;
};

Texture3D g_Perlin4D : register(t12);
Texture2D g_Interaction : register(t13);
StructuredBuffer<uint> visible_indices : register(t33);
StructuredBuffer<DetailModelGPU> detail_models : register(t35);
StructuredBuffer<PulledVertex> pulled_vertices : register(t36);
StructuredBuffer<DetailInstance> all_instances : register(t37);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    nointerpolation uint visID : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    uint tri = vid / 3u;
    DetailInstance raw = all_instances[visible_indices[iid]];

    VS_OUTPUT o;
    o.visID = PackVisID(g_EntryBase + (g_Kind << 22) + iid, tri);

    if (g_Kind >= DETAIL_KIND_MESH)
    {
        PulledInstance inst = DecodePulled(raw);
        DetailModelGPU mdl = detail_models[inst.objectId];
        if (vid >= mdl.pulledIndexCount)
        {
            o.position = asfloat(0x7FC00000);
            o.uv = float2(0.0, 0.0);
            return o;
        }
        PulledVertex v = pulled_vertices[mdl.pulledVertexBase + vid];
        float3 wp = PulledWorldPos(inst, v);
        if (g_Kind == DETAIL_KIND_MESH)
        {
            float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, inst.pos.xz, interaction_window);
            wp = PulledInteractionBend(inst, wp, inter, grass_interaction_displacement, grass_interaction_max_angle);
            wp = PulledSway(wp, PulledHeightFactor(v, mdl), wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
        }
        o.position = mul(m_VP, float4(wp, 1.0));
        o.uv = float2(v.u, v.v);
        return o;
    }

    uint corner = vid - tri * 3u;
    uint localVert = BladeTriangleVertex(tri, corner, g_Segments);
    BladeInstance b = DecodeBlade(raw, g_Perlin4D, smp_linear, grass_blade_height);
    float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, b.pos.xz, interaction_window);
    BladeBend w = EvalBladeBend(b, wave.w, g_wind_direction.xy, grass_wind_displacement, inter, grass_interaction_displacement, grass_interaction_max_angle, g_Perlin4D, smp_linear);
    BladeVertex v = EvalBladeVertex(b, w, localVert, g_Segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
    o.position = mul(m_VP, float4(v.pos, 1.0));
    o.uv = v.uv;
    return o;
}
