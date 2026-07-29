#ifndef SKINNED_MDI_COMMON_H
#define SKINNED_MDI_COMMON_H

#include "skinned_common.h"

struct SkinnedDrawRecord
{
    float4x4 world;
    uint boneOffset;
    uint splatOffset;
    uint splatCount;
    uint pad;
};

StructuredBuffer<SkinnedDrawRecord> g_SkinnedRecords : register(t14);
StructuredBuffer<uint> g_SkinnedCompactIndices : register(t15);
StructuredBuffer<uint> g_SkinnedCompactMaterialIDs : register(t16);

SkinnedDrawRecord skinned_mdi_record(uint drawIndex)
{
    return g_SkinnedRecords[g_SkinnedCompactIndices[drawIndex]];
}

uint skinned_mdi_material(uint drawIndex)
{
    return g_SkinnedCompactMaterialIDs[drawIndex];
}

float4x4 mdi_get_bone(uint boneOffset, int legacy_index)
{
    return g_BoneMatrices[boneOffset + (legacy_index / 3)];
}

float3 mdi_skin_splat_pos(uint boneOffset, PaintSplat splat)
{
    float3 result = 0;
    for (uint i = 0; i < 4; i++)
    {
        float4x4 bone = g_BoneMatrices[boneOffset + splat.boneIdx[i]];
        result += mul(bone, float4(splat.posRadius.xyz, 1)).xyz * splat.boneWeights[i];
    }
    return result;
}

float3 mdi_apply_splat_deform(SkinnedDrawRecord rec, float3 worldPos, float3 worldNormal)
{
    float3 offset = 0;
    for (uint i = 0; i < rec.splatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[rec.splatOffset + i];
        float3 splatWorldPos = mul(rec.world, float4(mdi_skin_splat_pos(rec.boneOffset, splat), 1)).xyz;
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

float3 mdi_apply_splat_color(SkinnedDrawRecord rec, float3 albedo, float3 worldPos, float2 meshUV)
{
    for (uint i = 0; i < rec.splatCount; i++)
    {
        PaintSplat splat = g_PaintSplats[rec.splatOffset + i];
        float3 splatWorldPos = mul(rec.world, float4(mdi_skin_splat_pos(rec.boneOffset, splat), 1)).xyz;
        float dist = distance(worldPos, splatWorldPos);
        float r = splat.posRadius.w;
        uint mode = get_splat_mode(splat);
        float reach = (mode == SPLAT_MODE_PROCEDURAL_BLOOD) ? (r * 1.8) : r;
        if (dist < reach)
        {
            float fade = 1.0 - smoothstep(r * 0.5, reach, dist);

            if (mode == SPLAT_MODE_PROCEDURAL_BLOOD)
            {
                float4 proc = sample_procedural_blood(splat, meshUV);
                if (proc.a <= 1e-5)
                    continue;
                float procAlpha = saturate(fade * proc.a);
                albedo = lerp(albedo, proc.rgb, procAlpha);
                continue;
            }

            float4 stamp = sample_decal_stamp(splat, meshUV);
            if (stamp.a <= 1e-5)
                continue;

            float3 splatColor = splat.color.rgb * stamp.rgb;
            float splatAlpha = splat.color.a * fade * stamp.a;
            albedo = lerp(albedo, splatColor, saturate(splatAlpha));
        }
    }
    return albedo;
}

struct VS_OUTPUT_MDI
{
    float4 position  : SV_Position;
    float3 worldPos  : TEXCOORD0;
    float2 texcoord  : TEXCOORD1;
    float3 normal    : TEXCOORD2;
    float3 tangent   : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    nointerpolation uint materialID : TEXCOORD5;
    nointerpolation uint drawIndex  : TEXCOORD6;
};

#endif
