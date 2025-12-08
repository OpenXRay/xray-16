#define SM_6_0
#include "common.h"
#include "bindless_common.h"

StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

float3x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[legacy_index / 3];
}

float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
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

struct VS_INPUT_1W
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
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

#define DEBUG_SKIP_SKINNING 0

VS_OUTPUT main(VS_INPUT_1W v)
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
    int boneIdx = int(v.N.w * 255.0 + 0.3);
    float3x4 bone = get_bone(boneIdx);

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);
#endif

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
