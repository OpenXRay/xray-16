#ifndef SKINNED_COMMON_H
#define SKINNED_COMMON_H

StructuredBuffer<float4x4> g_BoneMatrices : register(t3);

cbuffer SkinnedMaterialCB : register(b4)
{
    uint g_SkinnedMaterialID;
    uint g_SkeletonBoneOffset;
    uint g_SplatOffset;
    uint g_SplatCount;
};

struct PaintSplat
{
    float4 posRadius;       // xyz = rest-pose position, w = world-space radius
    float4 color;           // rgb + alpha
    uint4  boneIdx;         // up to 4 bone indices (local to skeleton)
    float4 boneWeights;     // corresponding weights (sum = 1)
};

StructuredBuffer<PaintSplat> g_PaintSplats : register(t11);

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

float3 skin_splat_pos(PaintSplat splat)
{
    float3 result = 0;
    for (uint i = 0; i < 4; i++)
    {
        float4x4 bone = g_BoneMatrices[g_SkeletonBoneOffset + splat.boneIdx[i]];
        result += mul(bone, float4(splat.posRadius.xyz, 1)).xyz * splat.boneWeights[i];
    }
    return result;
}

float3 apply_splat_deform(float3 worldPos, float3 worldNormal)
{
    float3 offset = 0;
    for (uint i = 0; i < g_SplatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[g_SplatOffset + i];
        float3 splatWorldPos = mul(m_W, float4(skin_splat_pos(splat), 1)).xyz;
        float dist = distance(worldPos, splatWorldPos);
        float r = splat.posRadius.w;
        if (dist < r)
        {
            float fade = 1.0 - smoothstep(r * 0.5, r, dist);
            offset -= worldNormal * (splat.color.a * fade * dev_param_1.x);
        }
    }
    return offset;
}

float3 apply_splat_color(float3 albedo, float3 worldPos)
{
    for (uint i = 0; i < g_SplatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[g_SplatOffset + i];
        float3 splatWorldPos = mul(m_W, float4(skin_splat_pos(splat), 1)).xyz;
        float dist = distance(worldPos, splatWorldPos);
        float r = splat.posRadius.w;
        if (dist < r)
        {
            float fade = 1.0 - smoothstep(r * 0.5, r, dist);
            albedo = lerp(albedo, splat.color.rgb, splat.color.a * fade);
        }
    }
    return albedo;
}

#endif // SKINNED_COMMON_H
