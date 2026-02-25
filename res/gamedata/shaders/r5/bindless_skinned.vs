// bindless_skinned.vs
// GPU-driven skinned mesh shader for vertHW_1W format (24 bytes, 1-bone)

#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT
{
    float4 P  : POSITION;   // SHORT4: quantized position
    float4 N  : NORMAL;     // D3DCOLOR: normal, .w = bone index
    float4 T  : TANGENT;    // D3DCOLOR: tangent
    float4 B  : BINORMAL;   // D3DCOLOR: binormal
    float2 tc : TEXCOORD0;  // SHORT2: UV
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

float4 unpack_skinned_position(float4 v)
{
    return float4(v.xyz * 12.0, 1.0);
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 localPos = unpack_skinned_position(input.P);
    float3 N = unpack_d3dcolor_normal(input.N.xyz);
    float3 T = unpack_d3dcolor_normal(input.T.xyz);
    float3 B = unpack_d3dcolor_normal(input.B.xyz);

    int boneIdx = int(input.N.w * 255.0 + 0.3);
    float4x4 bone = get_bone(boneIdx);

    float4 skinnedPos = skinning_pos(localPos, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);

    skinnedPos.xyz = apply_overlay_deform(skinnedPos.xyz, skinnedN, input.tc);

    float3 worldPos3 = mul(m_W, skinnedPos).xyz;
    output.worldPos = worldPos3;
    output.position = mul(m_VP, float4(worldPos3, 1.0));

    float3x3 worldRot = (float3x3)m_W;
    output.normal = normalize(mul(worldRot, skinnedN));
    output.tangent = normalize(mul(worldRot, skinnedT));
    output.bitangent = normalize(mul(worldRot, skinnedB));

    output.materialID = g_SkinnedMaterialID;
    output.texcoord = input.tc;

    return output;
}
