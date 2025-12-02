// bindless_depth.vs
// Depth-only vertex shader for bindless geometry
// MUST produce identical SV_Position as bindless_forward.vs for Z consistency

#define SM_5_0
#include "common.h"

// UnifiedVertex format (48 bytes) - same as bindless_forward.vs
struct VS_INPUT
{
    float4 position  : POSITION;     // float3 position
    float4 normal    : NORMAL;       // D3DCOLOR: packed normal + hemi (unused for depth)
    float4 tangent   : TANGENT;      // D3DCOLOR: packed tangent (unused for depth)
    float4 binormal  : BINORMAL;     // D3DCOLOR: packed binormal (unused for depth)
    float2 texcoord  : TEXCOORD0;    // float2: base UV (unused for depth)
    float2 texcoord1 : TEXCOORD1;    // float2: lightmap UV (unused for depth)
    float4 color     : COLOR0;       // D3DCOLOR: vertex color (unused for depth)
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
};

// Per-draw constants - MUST match bindless_forward.vs exactly
cbuffer PerDrawConstants : register(b5)
{
    float4x4 g_World;
    uint g_MaterialID;
    float3 g_Padding;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Transform position - IDENTICAL to bindless_forward.vs
    float4 worldPos = mul(g_World, float4(input.position.xyz, 1.0));
    output.position = mul(m_VP, worldPos);

    return output;
}
