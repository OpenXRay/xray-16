// bindless_forward.vs
// SM6 Bindless forward vertex shader
//
// SUPPORTS TWO VERTEX FORMATS:
// 1. Legacy X-Ray compressed format (32 bytes) - current rendering path
// 2. UnifiedVertex format (48 bytes) - GPU-driven mega-buffer path
//
// For now, we use the UnifiedVertex format which has pre-unpacked UVs.
// When rendering from legacy D3D11 buffers, the input layout handles conversion.

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// UnifiedVertex format (48 bytes):
//   Position:  float3    at offset  0 (12 bytes)
//   Normal:    D3DCOLOR  at offset 12 (4 bytes) - packed [-1,1] -> [0,1], A=hemi
//   Tangent:   D3DCOLOR  at offset 16 (4 bytes) - packed
//   Binormal:  D3DCOLOR  at offset 20 (4 bytes) - packed
//   TexCoord0: float2    at offset 24 (8 bytes) - pre-unpacked base UV
//   TexCoord1: float2    at offset 32 (8 bytes) - lightmap UV
//   Color:     D3DCOLOR  at offset 40 (4 bytes) - vertex color
//   Flags:     uint      at offset 44 (4 bytes) - reserved
struct VS_INPUT
{
    float4 position  : POSITION;     // float3 position
    float4 normal    : NORMAL;       // D3DCOLOR: packed normal + hemi (BGRA8_UNORM)
    float4 tangent   : TANGENT;      // D3DCOLOR: packed tangent
    float4 binormal  : BINORMAL;     // D3DCOLOR: packed binormal
    float2 texcoord  : TEXCOORD0;    // float2: pre-unpacked base UV
    float2 texcoord1 : TEXCOORD1;    // float2: lightmap UV
    float4 color     : COLOR0;       // D3DCOLOR: vertex color (BGRA8_UNORM)
    uint drawIndex   : DRAWINDEX;    // Per-instance draw index from StartInstanceLocation
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;  // Direct material ID (no indirection)
};

// ═══════════════════════════════════════════════════════
//  GPU-DRIVEN RENDERING BUFFERS
// ═══════════════════════════════════════════════════════
// Instance data: world matrix + materialID per batch
struct InstanceData
{
    float4x4 world;     // World transform (64 bytes)
    uint materialID;    // Bindless material ID
    uint flags;         // Instance flags
    float pad0, pad1;   // Padding to 80 bytes
};

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_CompactBatchIndices : register(t15);
StructuredBuffer<uint> g_CompactMaterialIDs : register(t16);

// Unpack D3DCOLOR normal from [0,1] to [-1,1]
float3 UnpackNormal(float4 packed)
{
    // D3DCOLOR stores in BGRA order, but BGRA8_UNORM reads as RGBA
    // So .rgb gives us the normal components
    return packed.rgb * 2.0 - 1.0;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // GPU-driven multi-draw: Draw index comes from per-instance DRAWINDEX attribute
    // The compaction shader sets StartInstanceLocation in indirect args (0,1,2,3...)
    // D3D12 uses StartInstanceLocation to offset into the per-instance draw index buffer
    uint drawID = input.drawIndex;

    uint batchIndex = g_CompactBatchIndices[drawID];
    InstanceData instanceData = g_InstanceData[batchIndex];
    float4x4 worldMatrix = instanceData.world;
    uint materialID = g_CompactMaterialIDs[drawID];

    // Unpack compressed normal/tangent/binormal
    float3 normalUnpacked = UnpackNormal(input.normal);
    float3 tangentUnpacked = UnpackNormal(input.tangent);
    float3 binormalUnpacked = UnpackNormal(input.binormal);

    // Transform position
    float4 worldPos = mul(worldMatrix, float4(input.position.xyz, 1.0));
    output.worldPos = worldPos.xyz;
    float3 clipPos = worldPos.xyz;
    if (g_Materials[materialID].flags & MAT_FLAG_ALPHA_BLEND)
        clipPos += (eye_position - clipPos) * 0.002;
    output.position = mul(m_VP, float4(clipPos, 1.0));

    // Transform normal/tangent to world space
    float3x3 worldMatrix3x3 = (float3x3)worldMatrix;
    output.normal = normalize(mul(worldMatrix3x3, normalUnpacked));
    output.tangent = normalize(mul(worldMatrix3x3, tangentUnpacked));
    output.bitangent = normalize(mul(worldMatrix3x3, binormalUnpacked));

    // UVs are pre-unpacked in UnifiedVertex format - pass through directly
    output.texcoord = input.texcoord;

    // Pass material ID to pixel shader
    output.materialID = materialID;

    return output;
}
