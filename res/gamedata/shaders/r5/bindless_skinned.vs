#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct VS_INPUT
{
    float4 P        : POSITION;
    float4 N        : NORMAL;
    float4 T        : TANGENT;
    float4 B        : BINORMAL;
    float2 tc       : TEXCOORD0;
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

cbuffer BonesCB : register(b3)
{
    float3x4 sbones_array[78];
};

float3x4 get_bone(int legacy_index)
{
    return sbones_array[legacy_index / 3];
}

float3 unpack_skinned_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
}

float4 skinning_pos(float4 pos, float3x4 bone)
{
    return float4(mul(bone, pos), 1.0);
}

float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

float4 unpack_skinned_position(float4 v)
{
    return float4(v.xyz * 12.0, 1.0);
}

#define DEBUG_SKIP_SKINNING 1

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 localPos = unpack_skinned_position(input.P);

    float3 normalUnpacked = unpack_skinned_normal(input.N.xyz);
    float3 tangentUnpacked = unpack_skinned_normal(input.T.xyz);
    float3 binormalUnpacked = unpack_skinned_normal(input.B.xyz);

#if DEBUG_SKIP_SKINNING
    float4 skinnedPos = localPos;
    float3 skinnedNormal = normalUnpacked;
    float3 skinnedTangent = tangentUnpacked;
    float3 skinnedBinormal = binormalUnpacked;
#else
    int boneIdx = int(input.N.w * 255.0 + 0.3);
    float3x4 bone = get_bone(boneIdx);

    float4 skinnedPos = skinning_pos(localPos, bone);
    float3 skinnedNormal = skinning_dir(normalUnpacked, bone);
    float3 skinnedTangent = skinning_dir(tangentUnpacked, bone);
    float3 skinnedBinormal = skinning_dir(binormalUnpacked, bone);
#endif

    float3 worldPos3 = mul(m_W, skinnedPos);
    float4 worldPos = float4(worldPos3, 1.0);
    output.worldPos = worldPos3;
    output.position = mul(m_VP, worldPos);

    float3x3 worldMatrix3x3 = (float3x3)m_W;
    output.normal = normalize(mul(worldMatrix3x3, skinnedNormal));
    output.tangent = normalize(mul(worldMatrix3x3, skinnedTangent));
    output.bitangent = normalize(mul(worldMatrix3x3, skinnedBinormal));

    output.texcoord = input.tc;

    output.materialID = 0;

    return output;
}
