// bindless_skinned_2w.vs
// SM6 Bindless vertex shader for 2-bone skinned meshes (2W_HQ format, 44 bytes)
// Matches vanilla skin.h skinning_2() exactly
//
// Layout:
//   FLOAT4 POSITION at 0 (16 bytes)
//   D3DCOLOR NORMAL at 16 (4 bytes) - .w = weight
//   D3DCOLOR TANGENT at 20 (4 bytes)
//   D3DCOLOR BINORMAL at 24 (4 bytes)
//   FLOAT4 TEXCOORD at 28 (16 bytes) - .xy = UV, .zw = bone indices

#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// Bone matrices as StructuredBuffer (t3 SRV)
StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

float3x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[legacy_index / 3];
}

float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;   // [0,1] -> [-1,1]
}

float4 skinning_pos(float4 pos, float3x4 bone)
{
    float4 P = float4(pos.xyz, 1.0);
    return float4(mul(bone, P), 1.0);
}

float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

struct VS_INPUT_2W
{
    float4 P  : POSITION;
    float4 N  : NORMAL;     // .xyz = normal, .w = blend weight
    float4 T  : TANGENT;    // .xyz = tangent
    float4 B  : BINORMAL;   // .xyz = binormal
    float4 tc : TEXCOORD0;  // .xy = UV, .zw = bone indices (as floats, already *3)
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

#define DEBUG_SKIP_SKINNING 0

VS_OUTPUT main(VS_INPUT_2W v)
{
    VS_OUTPUT o;

    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

#if DEBUG_SKIP_SKINNING
    float4 skinnedPos = float4(v.P.xyz, 1.0);
    float3 skinnedN = N;
    float3 skinnedT = T;
    float3 skinnedB = B;
#else
    // Bone indices from tc.zw (stored as floats, already *3 from SHORT conversion)
    // Note: Unlike D3DCOLOR indices, these are direct values, no *255 scaling
    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);

    float3x4 bone_0 = get_bone(id_0);
    float3x4 bone_1 = get_bone(id_1);

    // Weight from N.w (D3DCOLOR 0-1 range)
    float w = v.N.w;

    // Blend 2 bones
    float3x4 bone = lerp(bone_0, bone_1, w);

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);
#endif

    // Transform to world space
    float3 worldPos3 = mul(m_W, skinnedPos);
    o.worldPos = worldPos3;
    o.position = mul(m_VP, float4(worldPos3, 1.0));

    float3x3 worldMatrix3x3 = (float3x3)m_W;
    o.normal = normalize(mul(worldMatrix3x3, skinnedN));
    o.tangent = normalize(mul(worldMatrix3x3, skinnedT));
    o.bitangent = normalize(mul(worldMatrix3x3, skinnedB));

    o.texcoord = v.tc.xy;
    o.materialID = 0;

    return o;
}
