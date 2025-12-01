// cull_debug.vs - Debug visualization vertex shader
// Renders bounding spheres as billboarded quads (circles drawn in PS)
//
// Uses instancing: 4 vertices per quad, 1 instance per object
// SV_VertexID: 0=BL, 1=BR, 2=TL, 3=TR (triangle strip)
//
#define SM_5_0
#include "common.h"
#include "cull_debug.h"

// ═══════════════════════════════════════════════════════
//  INPUT
// ═══════════════════════════════════════════════════════

// Debug data from compute shader (one per object)
StructuredBuffer<CullDebugData> g_DebugData : register(t0);

cbuffer CullDebugVSParams : register(b5)
{
    float4x4 g_View;       // View matrix (for billboard orientation)
    float4x4 g_ViewProj;   // View-projection matrix
    uint g_ObjectCount;    // Number of objects
    float g_WireframeAlpha; // Wireframe transparency
    float2 g_Padding;
};

// ═══════════════════════════════════════════════════════
//  OUTPUT
// ═══════════════════════════════════════════════════════

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD0;     // For circle rendering in PS
    float  alpha    : TEXCOORD1;
    uint   cullState: TEXCOORD2;
};

// ═══════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;

    // Bounds check
    if (instanceID >= g_ObjectCount)
    {
        output.position = float4(0, 0, 0, 0);
        output.color = float3(0, 0, 0);
        output.uv = float2(0, 0);
        output.alpha = 0;
        output.cullState = 0;
        return output;
    }

    // Load debug data for this object
    CullDebugData debugData = g_DebugData[instanceID];

    // Quad corner offsets (in local billboard space)
    // VertexID: 0=BL, 1=BR, 2=TL, 3=TR
    float2 cornerOffset;
    switch (vertexID)
    {
        case 0: cornerOffset = float2(-1, -1); break; // Bottom-left
        case 1: cornerOffset = float2( 1, -1); break; // Bottom-right
        case 2: cornerOffset = float2(-1,  1); break; // Top-left
        case 3: cornerOffset = float2( 1,  1); break; // Top-right
        default: cornerOffset = float2(0, 0); break;
    }

    // Extract camera right and up vectors from view matrix
    // X-Ray uses row-major: M[row][col], so rows contain the camera axes
    // Row 0 = right, Row 1 = up, Row 2 = forward
    float3 camRight = float3(g_View[0][0], g_View[0][1], g_View[0][2]);
    float3 camUp    = float3(g_View[1][0], g_View[1][1], g_View[1][2]);

    // Calculate world-space billboard vertex position
    float3 worldPos = debugData.position
                    + camRight * cornerOffset.x * debugData.radius
                    + camUp    * cornerOffset.y * debugData.radius;

    // Transform to clip space (use our cbuffer, not common.h's m_VP)
    output.position = mul(g_ViewProj, float4(worldPos, 1.0));

    // Get debug color based on cull state
    output.color = GetCullDebugColor(debugData.cullState);
    output.cullState = debugData.cullState;

    // UV for circle rendering ([-1, 1] range)
    output.uv = cornerOffset;

    // Alpha based on cull state
    // Hide visible objects (green) - only show culled objects
    if (debugData.cullState == CULL_STATE_VISIBLE)
    {
        output.alpha = 0.0; // Hide visible objects
    }
    else if (debugData.cullState >= CULL_STATE_CULLED_DISTANCE)
    {
        output.alpha = g_WireframeAlpha; // Culled = full visibility
    }
    else
    {
        output.alpha = g_WireframeAlpha * 0.5; // Occluders = semi-transparent
    }

    return output;
}
