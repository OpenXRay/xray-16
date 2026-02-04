// skinned_common.h
// Shared definitions for GPU-driven skinned mesh rendering

#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

// Global bone buffer - contains all skeleton bones for the frame
// Each skeleton has a contiguous range at [skeletonBoneOffset, skeletonBoneOffset + boneCount)
StructuredBuffer<float3x4> g_BoneMatrices : register(t3);

// Per-instance skinned data
struct SkinnedInstanceData {
    float4x4 world;
    uint materialID;
    uint skeletonBoneOffset;
    uint batchIndex;
    uint flags;
};
StructuredBuffer<SkinnedInstanceData> g_SkinnedInstances : register(t4);

// Get bone matrix using per-instance offset
float3x4 get_bone(int legacy_index, uint instanceID)
{
    uint baseOffset = g_SkinnedInstances[instanceID].skeletonBoneOffset;
    return g_BoneMatrices[baseOffset + (legacy_index / 3)];
}

// Transform position by bone matrix
float4 skinning_pos(float4 pos, float3x4 bone)
{
    return float4(mul(bone, pos), 1.0);
}

// Transform direction by bone matrix (no translation)
float3 skinning_dir(float3 dir, float3x4 bone)
{
    return mul((float3x3)bone, dir);
}

// Unpack D3DCOLOR normal: [0,1] -> [-1,1]
// Note: BGRA8_UNORM format in input layout handles BGR->RGB automatically
float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
}

#endif // SKINNED_COMMON_H
