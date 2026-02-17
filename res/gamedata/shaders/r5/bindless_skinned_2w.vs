// bindless_skinned_2w.vs
// GPU-driven skinned mesh shader for 2W HQ format (44 bytes, 2-bone blend)
// Weight formula: lerp(bone_0, bone_1, N.w) = bone_0 * (1 - N.w) + bone_1 * N.w

#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_2W
{
    float4 P  : POSITION;   // FLOAT4: position
    float4 N  : NORMAL;     // D3DCOLOR: normal, .w = lerp weight
    float4 T  : TANGENT;    // D3DCOLOR: tangent
    float4 B  : BINORMAL;   // D3DCOLOR: binormal
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

VS_OUTPUT main(VS_INPUT_2W v)
{
    VS_OUTPUT o;

    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);

    float3x4 bone_0 = get_bone(id_0);
    float3x4 bone_1 = get_bone(id_1);

    // 2W uses lerp: result = bone_0 * (1-w) + bone_1 * w
    float w = v.N.w;
    float3x4 bone = lerp(bone_0, bone_1, w);

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);

    float3 worldPos3 = mul(m_W, skinnedPos);
    o.worldPos = worldPos3;
    o.position = mul(m_VP, float4(worldPos3, 1.0));

    float3x3 worldRot = (float3x3)m_W;
    o.normal = normalize(mul(worldRot, skinnedN));
    o.tangent = normalize(mul(worldRot, skinnedT));
    o.bitangent = normalize(mul(worldRot, skinnedB));

    o.materialID = g_SkinnedMaterialID;
    o.texcoord = v.tc.xy;

    return o;
}
