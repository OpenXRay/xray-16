// bindless_skinned.vs
// SM6 Bindless forward vertex shader for SKINNED meshes
// Applies bone transforms using sbones_array before world transform
//
// Vertex format: vertHW_1W (24 bytes) - most common skinned format
//   Position:  SHORT4    at offset 0  (8 bytes) - quantized position
//   Normal:    D3DCOLOR  at offset 8  (4 bytes) - packed normal, .w = bone index
//   Tangent:   D3DCOLOR  at offset 12 (4 bytes) - packed tangent
//   Binormal:  D3DCOLOR  at offset 16 (4 bytes) - packed binormal
//   TexCoord:  SHORT2    at offset 20 (4 bytes) - quantized UV

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// Skinned vertex input (matches vertHW_1W / dwDecl_01W)
struct VS_INPUT
{
    float4 P        : POSITION;     // SHORT4: quantized position (-12..+12)
    float4 N        : NORMAL;       // D3DCOLOR: packed normal, .w = bone index (0..255)
    float4 T        : TANGENT;      // D3DCOLOR: packed tangent
    float4 B        : BINORMAL;     // D3DCOLOR: packed binormal
    float2 tc       : TEXCOORD0;    // SHORT2: quantized UV (-16..+16)
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
    float3 bitangent: TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
};

// Bone matrices - uploaded per skeleton instance as StructuredBuffer
// Using float3x4 format (3 rows of float4) - matches legacy sbones_array
// 78 bones * float3x4 (48 bytes each) = 3744 bytes
StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

// Get bone matrix from index
// Legacy X-Ray multiplies bone index by 3 (for 3 rows per matrix)
// The index stored in vertex is already pre-multiplied by 3
float3x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[legacy_index / 3];
}

// Unpack D3DCOLOR normal from [0,1] to [-1,1]
// color_rgba(r,g,b,a) stores as 0xAARRGGBB in memory (little-endian):
//   byte 0=B, byte 1=G, byte 2=R, byte 3=A
// BGRA8_UNORM reads: .x=R (byte 2), .y=G (byte 1), .z=B (byte 0), .w=A (byte 3)
// So normal.xyz in color_rgba becomes .xyz in shader - NO swizzle needed!
// NOTE: Named differently to avoid conflict with common_functions.h unpack_normal
float3 unpack_skinned_normal(float3 packed)
{
    return packed * 2.0 - 1.0;  // Just unpack from [0,1] to [-1,1]
}

// Transform position by bone matrix
float4 skinning_pos(float4 pos, float3x4 bone)
{
    // Position already has w=1 from quantization
    return float4(mul(bone, pos), 1.0);
}

// Transform direction by bone matrix (3x3 rotation only)
float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

// Unpack quantized skinned position from SHORT4 to world units
// Legacy uses position range [-12, +12] stored in [-32768, +32767]
// Input layout RGBA16_SNORM normalizes to [-1, +1], so multiply by 12
float4 unpack_skinned_position(float4 v)
{
    return float4(v.xyz * 12.0, 1.0);
}

// DEBUG: Set to 1 to skip bone skinning and just render base mesh
#define DEBUG_SKIP_SKINNING 0

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Unpack quantized position to world units
    float4 localPos = unpack_skinned_position(input.P);

    // Unpack normal/tangent/binormal (with BGRA swizzle)
    float3 normalUnpacked = unpack_skinned_normal(input.N.xyz);
    float3 tangentUnpacked = unpack_skinned_normal(input.T.xyz);
    float3 binormalUnpacked = unpack_skinned_normal(input.B.xyz);

#if DEBUG_SKIP_SKINNING
    // Skip bone skinning - just use local position directly
    float4 skinnedPos = localPos;
    float3 skinnedNormal = normalUnpacked;
    float3 skinnedTangent = tangentUnpacked;
    float3 skinnedBinormal = binormalUnpacked;
#else
    // Get bone index from normal.w (D3DCOLOR alpha channel)
    // Value is in [0,1] range, scale to [0,255] and it's already *3
    int boneIdx = int(input.N.w * 255.0 + 0.3);
    float3x4 bone = get_bone(boneIdx);

    // Apply bone skinning
    float4 skinnedPos = skinning_pos(localPos, bone);
    float3 skinnedNormal = skinning_dir(normalUnpacked, bone);
    float3 skinnedTangent = skinning_dir(tangentUnpacked, bone);
    float3 skinnedBinormal = skinning_dir(binormalUnpacked, bone);
#endif

    // Transform to world space using m_W (skeleton's world matrix)
    // m_W is float3x4 so mul returns float3, need to create float4 for m_VP
    float3 worldPos3 = mul(m_W, skinnedPos);
    float4 worldPos = float4(worldPos3, 1.0);
    output.worldPos = worldPos3;
    output.position = mul(m_VP, worldPos);

    // Transform normals to world space
    float3x3 worldMatrix3x3 = (float3x3)m_W;
    output.normal = normalize(mul(worldMatrix3x3, skinnedNormal));
    output.tangent = normalize(mul(worldMatrix3x3, skinnedTangent));
    output.bitangent = normalize(mul(worldMatrix3x3, skinnedBinormal));

    // UV: SHORT2 format needs unpacking
    // Legacy uses: tc * (1.0 / 32768.0) * 16.0 = tc * (16.0 / 32768.0)
    // But hardware already converts SHORT2 to float, so just pass through
    output.texcoord = input.tc;

    // Material ID: For skinned meshes, we use per-draw constant (not GPU-driven)
    // The material ID is set via FGConstantSystem in ForwardColorPassSetup
    // For now, use 0 - the C++ code will set the correct material via constants
    output.materialID = 0;

    return output;
}
