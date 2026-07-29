#define SM_6_0


#include "skinned_velocity_common.h"

struct SkinnedDrawRecord
{
    float4x4 world;
    uint boneOffset;
    uint splatOffset;
    uint splatCount;
    uint pad;
    float4x4 prevWorld;
    uint prevBoneOffset;
    uint prevValid;
    uint pad1;
    uint pad2;
};

StructuredBuffer<SkinnedDrawRecord> g_SkinnedRecords : register(t14);
StructuredBuffer<uint> g_SkinnedCompactIndices : register(t15);

struct VS_INPUT
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float2 tc : TEXCOORD0;
    uint drawIndex : DRAWINDEX;
};

float4x4 mdi_curr_bone(uint boneOffset, int legacy_index)
{
    return g_BoneMatrices[boneOffset + (legacy_index / 3)];
}

float4x4 mdi_prev_bone(SkinnedDrawRecord rec, int legacy_index)
{
    if (rec.prevValid)
        return g_PrevBoneMatrices[rec.prevBoneOffset + (legacy_index / 3)];
    return g_BoneMatrices[rec.boneOffset + (legacy_index / 3)];
}

VelocityVSOut main(VS_INPUT v)
{
    SkinnedDrawRecord rec = g_SkinnedRecords[g_SkinnedCompactIndices[v.drawIndex]];
    float4 localPos = float4(v.P.xyz * 12.0, 1.0);
    int boneIdx = int(v.N.w * 255.0 + 0.3);

    float3 currWorld = mul(rec.world, skinning_pos(localPos, mdi_curr_bone(rec.boneOffset, boneIdx))).xyz;
    float4x4 prevW = rec.prevValid ? rec.prevWorld : rec.world;
    float3 prevWorld = mul(prevW, skinning_pos(localPos, mdi_prev_bone(rec, boneIdx))).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
