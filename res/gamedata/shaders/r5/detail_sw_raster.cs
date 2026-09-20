#define SM_6_0
#include "common.h"
#include "visbuffer_common.h"
#include "detail_blade_common.h"
#include "sw_raster_common.h"
#include "sw_dispatch_common.h"

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
    uint buildDetailsBumpIndex;
};

cbuffer DetailSwParams : register(b5)
{
    uint g_EntryBase;
    uint g_Lod;
    uint g_Segments;
    uint g_PreparedCapacity;
    uint g_Width;
    uint g_Height;
    uint2 g_SwPad;
};

Texture3D g_Perlin4D : register(t12);
Texture2D g_Interaction : register(t13);
StructuredBuffer<uint> visible_indices : register(t33);
StructuredBuffer<DetailInstance> all_instances : register(t37);
StructuredBuffer<PreparedBlade> prepared_blades : register(t38);
ByteAddressBuffer g_DrawArgs : register(t39);

#define BLADE_MAX_VERTS 10

[numthreads(64, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    uint slot = SwDispatchLinearGroup(groupID) * 64u + groupThreadID.x;
    if (slot >= g_DrawArgs.Load(4) || !DetailSlotRepresentable(slot))
        return;

    DetailInstance raw = all_instances[visible_indices[slot]];
    BladeInstance b;
    BladeBend w;
    if (slot < g_PreparedCapacity)
    {
        PreparedBlade p = prepared_blades[slot];
        b = BladeFromPrepared(p, raw);
        w = BendFromPrepared(p, g_wind_direction.xy);
    }
    else
    {
        b = DecodeBlade(raw, g_Perlin4D, smp_linear, grass_blade_height);
        float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, b.pos.xz, interaction_window);
        w = EvalBladeBend(b, wave.w, g_wind_direction.xy, grass_wind_displacement, inter, grass_interaction_displacement, grass_interaction_max_angle, g_Perlin4D, smp_linear);
    }

    uint vertCount = g_Segments * 2u + 1u;
    float3 v[BLADE_MAX_VERTS];
    for (uint i = 0; i < vertCount; ++i)
    {
        BladeVertex bv = EvalBladeVertex(b, w, i, g_Segments, wave.w, grass_blade_width, g_Perlin4D, smp_linear);
        if (!SwProjectVertex(m_VP, bv.pos, g_Width, g_Height, v[i]))
            return;
    }

    uint entry = PackDetailEntry(g_EntryBase, g_Lod, slot);
    uint triCount = (g_Segments - 1u) * 2u + 1u;
    for (uint tri = 0; tri < triCount; ++tri)
    {
        float3 v0 = v[BladeTriangleVertex(tri, 0u, g_Segments)];
        float3 v1 = v[BladeTriangleVertex(tri, 1u, g_Segments)];
        float3 v2 = v[BladeTriangleVertex(tri, 2u, g_Segments)];
        if (SwEdgeFunction(v0.xy, v1.xy, v2.xy) < 0.0)
        {
            float3 t = v1;
            v1 = v2;
            v2 = t;
        }
        SwRasterizeTriangle(v0, v1, v2, g_Width, g_Height, PackVisID(entry, tri));
    }
}
