#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"

struct VS_INPUT
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float2 tc : TEXCOORD0;
    uint drawIndex : DRAWINDEX;
};

VS_OUTPUT_MDI main(VS_INPUT v)
{
    VS_OUTPUT_MDI o;

    SkinnedDrawRecord rec = skinned_mdi_record(v.drawIndex);

    float4 localPos = float4(v.P.xyz * 12.0, 1.0);
    float3 N = unpack_d3dcolor_normal(v.N.xyz);
    float3 T = unpack_d3dcolor_normal(v.T.xyz);
    float3 B = unpack_d3dcolor_normal(v.B.xyz);

    int boneIdx = int(v.N.w * 255.0 + 0.3);
    float4x4 bone = mdi_get_bone(rec.boneOffset, boneIdx);

    float4 skinnedPos = skinning_pos(localPos, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);

    float3x3 worldRot = (float3x3)rec.world;
    float3 worldPos3 = mul(rec.world, skinnedPos).xyz;
    o.normal = normalize(mul(worldRot, skinnedN));

    worldPos3 += mdi_apply_splat_deform(rec, worldPos3, o.normal);
    o.worldPos = worldPos3;
    o.position = mul(m_VP, float4(worldPos3, 1.0));
    o.tangent = normalize(mul(worldRot, skinnedT));
    o.bitangent = normalize(mul(worldRot, skinnedB));

    o.materialID = skinned_mdi_material(v.drawIndex);
    o.drawIndex = v.drawIndex;
    o.texcoord = v.tc;

    return o;
}
