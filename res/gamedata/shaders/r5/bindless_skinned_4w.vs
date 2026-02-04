// bindless_skinned_4w.vs
// GPU-driven skinned mesh shader for 4W format (40 bytes, 4-bone blend)

#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

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

VS_OUTPUT main(VS_INPUT_4W v, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT o;

    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = v.B.w;
    float w3 = 1.0 - w0 - w1 - w2;

    int id0 = int(v.ind.x * 255.0 + 0.3);
    int id1 = int(v.ind.y * 255.0 + 0.3);
    int id2 = int(v.ind.z * 255.0 + 0.3);
    int id3 = int(v.ind.w * 255.0 + 0.3);

    float3x4 bone = get_bone(id0, instanceID) * w0
                  + get_bone(id1, instanceID) * w1
                  + get_bone(id2, instanceID) * w2
                  + get_bone(id3, instanceID) * w3;

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
