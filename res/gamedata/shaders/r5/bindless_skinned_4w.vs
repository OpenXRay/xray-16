// bindless_skinned_4w.vs
// SM6 Bindless vertex shader for 4-bone skinned meshes (4W_HQ format, 40 bytes)
// Matches vanilla skin.h skinning_4() exactly
//
// Layout:
//   FLOAT4 POSITION at 0 (16 bytes)
//   D3DCOLOR NORMAL at 16 (4 bytes) - .w = weight0
//   D3DCOLOR TANGENT at 20 (4 bytes) - .w = weight1
//   D3DCOLOR BINORMAL at 24 (4 bytes) - .w = weight2
//   FLOAT2 TEXCOORD at 28 (8 bytes)
//   D3DCOLOR BLENDINDICES at 36 (4 bytes) - 4 bone indices

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

struct VS_INPUT_4W
{
    float4 P   : POSITION;
    float4 N   : NORMAL;        // .xyz = normal, .w = weight0
    float4 T   : TANGENT;       // .xyz = tangent, .w = weight1
    float4 B   : BINORMAL;      // .xyz = binormal, .w = weight2
    float2 tc  : TEXCOORD0;
    float4 ind : BLENDINDICES;  // 4 bone indices (D3DCOLOR)
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

VS_OUTPUT main(VS_INPUT_4W v)
{
    VS_OUTPUT o;

    // BGRA8_UNORM format already swizzles D3DCOLOR - no manual swizzle needed
    // Unpack normals from [0,1] to [-1,1]
    float3 N = v.N.xyz * 2.0 - 1.0;
    float3 T = v.T.xyz * 2.0 - 1.0;
    float3 B = v.B.xyz * 2.0 - 1.0;
    float4 ind = v.ind;

#if DEBUG_SKIP_SKINNING
    float4 skinnedPos = float4(v.P.xyz, 1.0);
    float3 skinnedN = N;
    float3 skinnedT = T;
    float3 skinnedB = B;
#else
    // Weights from N.w, T.w, B.w (D3DCOLOR 0-1 range)
    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = v.B.w;
    float w3 = 1.0 - w0 - w1 - w2;

    // Bone indices from BLENDINDICES (D3DCOLOR, scale by 255)
    int id0 = int(ind.x * 255.0 + 0.3);
    int id1 = int(ind.y * 255.0 + 0.3);
    int id2 = int(ind.z * 255.0 + 0.3);
    int id3 = int(ind.w * 255.0 + 0.3);

    // Blend 4 bones
    float3x4 bone = get_bone(id0) * w0
                  + get_bone(id1) * w1
                  + get_bone(id2) * w2
                  + get_bone(id3) * w3;

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

    o.texcoord = v.tc;
    o.materialID = 0;

    return o;
}
