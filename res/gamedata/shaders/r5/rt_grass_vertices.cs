#include "detail_blade_common.h"
#include "detail_source_common.h"
#include "sw_dispatch_common.h"

StructuredBuffer<uint2> g_VisibleIndices : register(t2);
Texture3D g_WindTexture : register(t3);
Texture2D g_Interaction : register(t4);
SamplerState smp_linear : register(s2);
SamplerState smp_rtlinear : register(s3);
RWByteAddressBuffer g_Output : register(u0);
RWByteAddressBuffer g_OutputIB : register(u1);

cbuffer GrassRTCB : register(b5) {
    float4 g_wind_direction;
    float4 wave;
    float grass_wind_displacement;
    float grass_blade_height;
    float grass_blade_width;
    uint segments;
    uint vertsPerBlade;
    uint bladeCount;
    uint outputVertexOffset;
    uint indicesPerBlade;
    uint outputIndexOffset;
    uint3 pad;
    float4 interaction_window;
    float grass_interaction_displacement;
    float grass_interaction_max_angle;
    float2 interactionPad;
};

uint pack_normal(float3 n)
{
    uint3 u = uint3(clamp(n * 127.5 + 127.5, 0, 255));
    return (u.x << 16) | (u.y << 8) | u.z;
}

[numthreads(256, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint global_vert = SwDispatchLinearGroup(group_id) * 256u + thread_id.x;
    uint blade_idx = global_vert / vertsPerBlade;
    uint local_vert = global_vert % vertsPerBlade;

    if (blade_idx >= bladeCount)
        return;

    DetailInstance raw = LoadDetailInstance(g_VisibleIndices[blade_idx]);

    BladeInstance b = DecodeBlade(raw, g_WindTexture, smp_linear, grass_blade_height);
    float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, b.pos.xz, interaction_window);
    BladeBend w = EvalBladeBend(b, wave.w, g_wind_direction.xy, grass_wind_displacement, inter, grass_interaction_displacement, grass_interaction_max_angle, g_WindTexture, smp_linear);
    BladeVertex v = EvalBladeVertex(b, w, local_vert, segments, wave.w, grass_blade_width, g_WindTexture, smp_linear);

    uint hash16 = uint(round(b.bladeHash * 65535.0)) & 0xFFFFu;
    uint sideBits = uint(round(v.uv.x * 2.0)) & 0x3u;

    uint outAddr = (outputVertexOffset + global_vert) * 24;
    g_Output.Store3(outAddr, asuint(v.pos));
    g_Output.Store(outAddr + 12, pack_normal(v.rotatedNormal1) | (b.objectId << 24) | (sideBits << 30));
    g_Output.Store(outAddr + 16, pack_normal(v.rotatedNormal2) | ((hash16 & 0xFFu) << 24));
    g_Output.Store(outAddr + 20, f32tof16(v.uv.y) | (((hash16 >> 8u) & 0xFFu) << 16));

    if (local_vert % 2 == 0 && local_vert < segments * 2) {
        uint seg = local_vert / 2;
        uint vBase = outputVertexOffset + blade_idx * vertsPerBlade;
        uint bladeIBByte = (outputIndexOffset + blade_idx * indicesPerBlade) * 4;
        if (seg < segments - 1) {
            uint byteOff = bladeIBByte + seg * 24;
            uint v = vBase + seg * 2;
            g_OutputIB.Store4(byteOff, uint4(v, v + 2, v + 1, v + 1));
            g_OutputIB.Store2(byteOff + 16, uint2(v + 2, v + 3));
        } else {
            uint byteOff = bladeIBByte + (segments - 1) * 24;
            uint tipV = vBase + (segments - 1) * 2;
            uint tipTop = vBase + segments * 2;
            g_OutputIB.Store3(byteOff, uint3(tipV, tipTop, tipV + 1));
        }
    }
}
