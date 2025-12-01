#define SM_5_0
#include "common.h"
#include "cull_debug.h"

StructuredBuffer<CullDebugData> g_DebugData : register(t0);

cbuffer CullDebugVSParams : register(b5)
{
    float4x4 g_View;
    float4x4 g_ViewProj;
    uint g_ObjectCount;
    float g_WireframeAlpha;
    float2 g_Padding;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD0;
    float  alpha    : TEXCOORD1;
    uint   cullState: TEXCOORD2;
};

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;

    if (instanceID >= g_ObjectCount)
    {
        output.position = float4(0, 0, 0, 0);
        output.color = float3(0, 0, 0);
        output.uv = float2(0, 0);
        output.alpha = 0;
        output.cullState = 0;
        return output;
    }

    CullDebugData debugData = g_DebugData[instanceID];

    float2 cornerOffset;
    switch (vertexID)
    {
        case 0: cornerOffset = float2(-1, -1); break;
        case 1: cornerOffset = float2( 1, -1); break;
        case 2: cornerOffset = float2(-1,  1); break;
        case 3: cornerOffset = float2( 1,  1); break;
        default: cornerOffset = float2(0, 0); break;
    }

    float3 camRight = float3(g_View[0][0], g_View[0][1], g_View[0][2]);
    float3 camUp    = float3(g_View[1][0], g_View[1][1], g_View[1][2]);

    float3 worldPos = debugData.position
                    + camRight * cornerOffset.x * debugData.radius
                    + camUp    * cornerOffset.y * debugData.radius;

    output.position = mul(g_ViewProj, float4(worldPos, 1.0));

    output.color = GetCullDebugColor(debugData.cullState);
    output.cullState = debugData.cullState;

    output.uv = cornerOffset;

    if (debugData.cullState == CULL_STATE_CULLED_OCCLUSION)
    {
        output.alpha = g_WireframeAlpha;
    }
    else if (debugData.cullState == CULL_STATE_PARTICLE_CULLED)
    {
        output.alpha = g_WireframeAlpha;
    }
    else
    {
        output.alpha = 0.0;
    }

    return output;
}
