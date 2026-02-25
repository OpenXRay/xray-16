#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

StructuredBuffer<float4x4> g_BoneMatrices : register(t3);

cbuffer SkinnedMaterialCB : register(b4)
{
    uint g_SkinnedMaterialID;
    uint g_SkeletonBoneOffset;
    uint g_OverlayTextureIndex;
    uint g_SkinnedPad1;
};

float4x4 get_bone(int legacy_index)
{
    return g_BoneMatrices[g_SkeletonBoneOffset + (legacy_index / 3)];
}

float4 skinning_pos(float4 pos, float4x4 bone)
{
    return mul(bone, pos);
}

float3 skinning_dir(float3 dir, float4x4 bone)
{
    return mul((float3x3)bone, dir);
}

float3 unpack_d3dcolor_normal(float3 packed)
{
    return packed * 2.0 - 1.0;
}

static const float OVERLAY_DEFORM_SCALE = -0.005;

float3 apply_overlay_deform(float3 pos, float3 normal, float2 uv)
{
    if (g_OverlayTextureIndex == 0xFFFFFFFF)
        return pos;

    Texture2D overlayTex = GetBindlessTexture(g_OverlayTextureIndex);
    float deform = saturate(overlayTex.SampleLevel(g_LinearSampler, uv, 0).a);
    return pos + normalize(normal) * (deform * OVERLAY_DEFORM_SCALE);
}

#endif // SKINNED_COMMON_H
