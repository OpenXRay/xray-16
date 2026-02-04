// bindless_skinned_3w.vs
// GPU-driven skinned mesh shader for 3W HQ format (44 bytes, 3-bone blend)
// Weight formula: bone_0 * N.w + bone_1 * T.w + bone_2 * (1 - N.w - T.w)

#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_3W
{
    float4 P  : POSITION;   // FLOAT4: position
    float4 N  : NORMAL;     // D3DCOLOR: normal, .w = weight0
    float4 T  : TANGENT;    // D3DCOLOR: tangent, .w = weight1
    float4 B  : BINORMAL;   // D3DCOLOR: binormal, .w = bone index 2
    float4 tc : TEXCOORD0;  // FLOAT4: .xy = UV, .zw = bone indices 0,1
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

VS_OUTPUT main(VS_INPUT_3W v, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT o;

    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

    // Bone indices: tc.zw for indices 0,1; B.w for index 2
    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    int id_2 = int(v.B.w * 255.0 + 0.3);  // D3DCOLOR format

    float3x4 bone_0 = get_bone(id_0, instanceID);
    float3x4 bone_1 = get_bone(id_1, instanceID);
    float3x4 bone_2 = get_bone(id_2, instanceID);

    // 3W weights: w0 = N.w, w1 = T.w, w2 = 1 - w0 - w1
    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = 1.0 - w0 - w1;

    // Blend 3 bones
    float3x4 bone = bone_0 * w0 + bone_1 * w1 + bone_2 * w2;

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
    o.texcoord = v.tc.xy;

    return o;
}
