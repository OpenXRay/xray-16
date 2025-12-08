// bindless_skinned_hq.vs
// SM6 Bindless forward vertex shader for SKINNED meshes (HQ format)
// Matches vanilla skin.h implementation exactly
//
// HQ formats use FLOAT4 positions (no SHORT4 quantization)
// All formats use D3DCOLOR (BGRA8_UNORM) for normals/tangents/binormals
//
// Supported formats:
//   - 1W_HQ (36 bytes): bone index in N.w (D3DCOLOR)
//   - 2W_HQ (44 bytes): weight in N.w, bone indices in tc.zw (FLOAT4)
//   - 3W_HQ (44 bytes): weights in N.w/T.w, bone indices in tc.zw + B.w
//   - 4W_HQ (40 bytes): weights in N.w/T.w/B.w, bone indices in BLENDINDICES

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// Bone matrices - uploaded per skeleton instance
// Using float3x4 format (3 rows of float4) - matches legacy sbones_array
cbuffer BonesCB : register(b3)
{
    float3x4 sbones_array[78];
}

// Get bone matrix from legacy index (pre-multiplied by 3)
float3x4 get_bone(int legacy_index)
{
    return sbones_array[legacy_index / 3];
}

// Unpack D3DCOLOR normal: BGRA8_UNORM -> float4, needs swizzle and remap to [-1,1]
// D3DCOLOR stores as BGRA, but HLSL reads as RGBA, so .xyz = .zyx
float3 unpack_d3dcolor_normal(float3 packed)
{
    float3 swizzled = packed.zyx;  // BGRA -> RGB order
    return swizzled * 2.0 - 1.0;   // [0,1] -> [-1,1]
}

// Transform position by bone matrix
float4 skinning_pos(float4 pos, float3x4 bone)
{
    float4 P = float4(pos.xyz, 1.0);
    return float4(mul(bone, P), 1.0);
}

// Transform direction by bone matrix (3x3 rotation only)
float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

// ═══════════════════════════════════════════════════════
//  1W FORMAT (36 bytes) - Single bone
// ═══════════════════════════════════════════════════════
// Input layout:
//   FLOAT4 POSITION at 0
//   D3DCOLOR NORMAL at 16 (bone_idx*3 in .w)
//   D3DCOLOR TANGENT at 20
//   D3DCOLOR BINORMAL at 24
//   FLOAT2 TEXCOORD at 28

struct VS_INPUT_1W
{
    float4 P  : POSITION;
    float4 N  : NORMAL;     // .xyz = normal (D3DCOLOR), .w = bone_idx*3 (0-1 range)
    float4 T  : TANGENT;    // .xyz = tangent (D3DCOLOR)
    float4 B  : BINORMAL;   // .xyz = binormal (D3DCOLOR)
    float2 tc : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position  : SV_Position;
    float3 worldPos  : TEXCOORD0;
    float2 texcoord  : TEXCOORD1;
    float3 normal    : TEXCOORD2;
    float3 tangent   : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
};

// DEBUG: Set to 1 to skip bone skinning and just render base mesh
#define DEBUG_SKIP_SKINNING 1

VS_OUTPUT main(VS_INPUT_1W v)
{
    VS_OUTPUT o;

    // Unpack normals (D3DCOLOR BGRA -> swizzle to RGB -> remap to [-1,1])
    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

#if DEBUG_SKIP_SKINNING
    // Skip bone skinning - just use local position directly
    float4 skinnedPos = float4(v.P.xyz, 1.0);
    float3 skinnedN = N;
    float3 skinnedT = T;
    float3 skinnedB = B;
#else
    // Get bone index from N.w (D3DCOLOR range 0-1, scale to 0-255)
    int boneIdx = int(v.N.w * 255.0 + 0.3);
    float3x4 bone = get_bone(boneIdx);

    // Apply single-bone skinning
    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);
#endif

    // Transform to world space using m_W (skeleton's world matrix)
    float3 worldPos3 = mul(m_W, skinnedPos);
    o.worldPos = worldPos3;
    o.position = mul(m_VP, float4(worldPos3, 1.0));

    // Transform normals to world space
    float3x3 worldMatrix3x3 = (float3x3)m_W;
    o.normal = normalize(mul(worldMatrix3x3, skinnedN));
    o.tangent = normalize(mul(worldMatrix3x3, skinnedT));
    o.bitangent = normalize(mul(worldMatrix3x3, skinnedB));

    // HQ format: UV is already in world units
    o.texcoord = v.tc;

    // Material ID: set via constant buffer (not GPU-driven for skinned meshes)
    o.materialID = 0;

    return o;
}
