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
    float2 hitUV;           // hit UV in target diffuse UV space
    float uvRadius;         // radius in UV space for stamp sampling
    uint wallmarkMaterialID; // bindless material ID for wallmark texture
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

float4 sample_splat_stamp(PaintSplat splat, float2 meshUV)
{
    if (splat.wallmarkMaterialID == INVALID_TEXTURE_INDEX || splat.uvRadius <= 1e-5)
        return float4(1, 1, 1, 1);

    float2 uvMin = splat.hitUV - splat.uvRadius;
    float2 uvMax = splat.hitUV + splat.uvRadius;
    if (any(meshUV < uvMin) || any(meshUV > uvMax))
        return float4(0, 0, 0, 0);

    float2 stampUV = (meshUV - uvMin) / max(2.0 * splat.uvRadius, 1e-5);
    MaterialData wallmarkMat = g_Materials[splat.wallmarkMaterialID];
    return SampleDiffuse(wallmarkMat, stampUV);
}

float3 apply_splat_color(float3 albedo, float3 worldPos, float2 meshUV)
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
            float3 splatColor = splat.color.rgb;
            float splatAlpha = splat.color.a * fade;

            float4 stamp = sample_splat_stamp(splat, meshUV);
            if (stamp.a <= 1e-5)
                continue;

            splatColor *= stamp.rgb;
            splatAlpha *= stamp.a;
            albedo = lerp(albedo, splatColor, saturate(splatAlpha));
        }
    }
    return albedo;
}

#endif // SKINNED_COMMON_H
