#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

struct SkinnedInstanceData {
    float4x4 world;
    uint materialID;
    uint skeletonBoneOffset;
    uint batchIndex;
    uint flags;
};
StructuredBuffer<SkinnedInstanceData> g_SkinnedInstances : register(t4);

float3x4 get_bone(int legacy_index, uint instanceID)
{
    uint baseOffset = g_SkinnedInstances[instanceID].skeletonBoneOffset;
    return g_BoneMatrices[baseOffset + (legacy_index / 3)];
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

#endif
