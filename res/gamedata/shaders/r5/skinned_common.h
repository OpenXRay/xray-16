#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

cbuffer SkinnedMaterialCB : register(b4)
{
    uint g_SkinnedMaterialID;
    uint g_SkeletonBoneOffset;
    uint g_SkinnedPad0;
    uint g_SkinnedPad1;
};

float3x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[g_SkeletonBoneOffset + (legacy_index / 3)];
}

float4 skinning_pos(float4 pos, float3x4 bone)
{
    return float4(mul(bone, pos), 1.0);
}

float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
}

#endif // SKINNED_COMMON_H
