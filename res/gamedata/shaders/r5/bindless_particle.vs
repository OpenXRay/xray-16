// bindless_particle.vs
// SM6 Bindless particle vertex shader
// Batched rendering: materialID per-vertex for single draw call

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// Particle vertex format (28 bytes):
//   Position: float3 at offset 0 (12 bytes) - WORLD SPACE
//   Color: D3DCOLOR at offset 12 (4 bytes)
//   Texcoord: float2 at offset 16 (8 bytes)
//   MaterialID: uint at offset 24 (4 bytes)
struct VS_INPUT
{
    float4 position : POSITION;
    float4 color    : COLOR0;
    float2 texcoord : TEXCOORD0;
    uint materialID : MATERIALID;
};

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color    : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    nointerpolation uint materialID : TEXCOORD3;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = float4(input.position.xyz, 1.0);
    output.hpos = mul(m_VP, worldPos);
    output.worldPos = worldPos.xyz;
    output.texcoord = input.texcoord;
    output.color = input.color;
    output.materialID = input.materialID;

    return output;
}
