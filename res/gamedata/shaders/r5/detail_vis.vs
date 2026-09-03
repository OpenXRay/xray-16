#define SM_6_0
#include "common.h"
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

cbuffer DetailVisParams : register(b5)
{
    uint g_EntryBase;
    uint g_Lod;
    uint g_Segments;
    uint g_VisPad;
};

Texture3D g_Perlin4D : register(t12);
StructuredBuffer<uint> visible_indices : register(t33);
StructuredBuffer<DetailInstance> all_instances : register(t37);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    nointerpolation uint visID : TEXCOORD0;
};

VS_OUTPUT main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    uint tri = vid / 3u;
    uint corner = vid - tri * 3u;
    uint localVert = BladeTriangleVertex(tri, corner, g_Segments);

    DetailInstance raw = all_instances[visible_indices[iid]];
    BladeInstance b = DecodeBlade(raw, g_Perlin4D, smp_linear, grass_blade_height);
    BladeWind w = EvalBladeWind(b, wave.w, g_wind_direction.xy, grass_wind_displacement, g_Perlin4D, smp_linear);
    BladeVertex v = EvalBladeVertex(b, w, localVert, g_Segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);

    VS_OUTPUT o;
    o.position = mul(m_VP, float4(v.pos, 1.0));
    o.visID = PackVisID(g_EntryBase + (g_Lod << 22) + iid, tri);
    return o;
}
