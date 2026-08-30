#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"
#include "vsm_common.h"
#include "vsm_params.h"
#include "vsm_skinned_route.h"

struct VS_INPUT_1W
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float2 tc : TEXCOORD0;
    uint drawIndex : DRAWINDEX;
};

VS_OUTPUT main(VS_INPUT_1W v)
{
    SkinnedDrawRecord rec = skinned_mdi_record(v.drawIndex / uint(VSM_SKIN_CAP));

    float3 N = unpack_d3dcolor_normal(v.N.xyz);

    int boneIdx = int(v.N.w * 255.0 + 0.3);
    float4x4 bone = mdi_get_bone(rec.boneOffset, boneIdx);

    float4 skinnedPos = skinning_pos(v.P, bone);
    float3 skinnedN = skinning_dir(N, bone);

    float3x3 worldRot = (float3x3)rec.world;
    float3 worldPos3 = mul(rec.world, skinnedPos).xyz;
    float3 normal = normalize(mul(worldRot, skinnedN));
    worldPos3 += mdi_apply_splat_deform(rec, worldPos3, normal);

    return VsmSkinnedOut(v.drawIndex, worldPos3, v.tc);
}
