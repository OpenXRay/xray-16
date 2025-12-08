// bindless_particle.vs
// SM6 Bindless particle vertex shader
// Particles use a simple 24-byte vertex format with pre-transformed world positions

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// Particle vertex format (24 bytes):
//   Position: float3 at offset 0 (12 bytes) - WORLD SPACE
//   Color: D3DCOLOR at offset 12 (4 bytes)
//   Texcoord: float2 at offset 16 (8 bytes)
struct VS_INPUT
{
    float4 position : POSITION;   // World-space position (billboard vertex)
    float4 color    : COLOR0;     // Vertex color (BGRA8_UNORM)
    float2 texcoord : TEXCOORD0;  // Texture coordinates
};

struct VS_OUTPUT
{
    float4 hpos     : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color    : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    nointerpolation uint materialID : TEXCOORD3;
};

// Particle material CB (b4)
cbuffer ParticleMaterialCB : register(b4)
{
    uint g_ParticleMaterialID;
    uint g_ParticleTextureIndex;  // Direct index into ResourceDescriptorHeap (unused if using MaterialData)
    uint2 g_ParticlePad;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Particle positions are already in world space (billboard generation done on CPU)
    // Just transform by VP matrix
    float4 worldPos = float4(input.position.xyz, 1.0);
    output.hpos = mul(m_VP, worldPos);
    output.worldPos = worldPos.xyz;

    // Pass through UVs
    output.texcoord = input.texcoord;

    // Unpack D3DCOLOR: BGRA8_UNORM reads as RGBA in shader
    // The color comes in as BGRA packed, but BGRA8_UNORM format handles swizzling
    output.color = input.color;

    // Pass material ID to pixel shader
    output.materialID = g_ParticleMaterialID;

    return output;
}
