// bindless_skinned_hq.vs
// GPU-driven skinned mesh shader for 1W_HQ format (36 bytes, 1-bone)

#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_1W
{
    float4 P  : POSITION;   // FLOAT4: position (HQ format, no quantization)
    float4 N  : NORMAL;     // D3DCOLOR: normal, .w = bone index
    float4 T  : TANGENT;    // D3DCOLOR: tangent
    float4 B  : BINORMAL;   // D3DCOLOR: binormal
    float2 tc : TEXCOORD0;  // FLOAT2: UV
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

VS_OUTPUT main(VS_INPUT_1W v, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT o;

    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

    int boneIdx = int(v.N.w * 255.0 + 0.3);
    float3x4 bone = get_bone(boneIdx, instanceID);

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);

    float4x4 worldMatrix = g_SkinnedInstances[instanceID].world;
    float3 worldPos3 = mul(worldMatrix, skinnedPos).xyz;
    o.worldPos = worldPos3;
    o.position = mul(m_VP, float4(worldPos3, 1.0));

    float3x3 worldMatrix3x3 = (float3x3)worldMatrix;
    o.normal = normalize(mul(worldMatrix3x3, skinnedN));
    o.tangent = normalize(mul(worldMatrix3x3, skinnedT));
    o.bitangent = normalize(mul(worldMatrix3x3, skinnedB));

    o.materialID = g_SkinnedInstances[instanceID].materialID;
    o.texcoord = v.tc;

    return o;
}
