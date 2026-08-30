#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "vsm_skinned_route.h"

struct VS_INPUT_3W
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float4 tc : TEXCOORD0;
    uint drawIndex : DRAWINDEX;
};

VS_OUTPUT main(VS_INPUT_3W v)
{
    SkinnedDrawRecord rec = skinned_mdi_record(v.drawIndex / uint(VSM_SKIN_CAP));

    float3 N = unpack_d3dcolor_normal(v.N.xyz);

    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    int id_2 = int(v.B.w * 255.0 + 0.3);

    float4x4 bone_0 = mdi_get_bone(rec.boneOffset, id_0);
    float4x4 bone_1 = mdi_get_bone(rec.boneOffset, id_1);
    float4x4 bone_2 = mdi_get_bone(rec.boneOffset, id_2);

    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = 1.0 - w0 - w1;

    float4x4 bone = bone_0 * w0 + bone_1 * w1 + bone_2 * w2;

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);

    float3x3 worldRot = (float3x3)rec.world;
    float3 worldPos3 = mul(rec.world, skinnedPos).xyz;
    float3 normal = normalize(mul(worldRot, skinnedN));
    worldPos3 += mdi_apply_splat_deform(rec, worldPos3, normal);

    return VsmSkinnedOut(v.drawIndex, worldPos3, v.tc.xy);
}
