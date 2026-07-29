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
    float4 tc : TEXCOORD0;
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
    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    float w = v.N.w;
    float4x4 bone = lerp(mdi_curr_bone(rec.boneOffset, id_0), mdi_curr_bone(rec.boneOffset, id_1), w);
    float4x4 prevBone = lerp(mdi_prev_bone(rec, id_0), mdi_prev_bone(rec, id_1), w);

    float3 currWorld = mul(rec.world, skinning_pos(v.P, bone)).xyz;
    float4x4 prevW = rec.prevValid ? rec.prevWorld : rec.world;
    float3 prevWorld = mul(prevW, skinning_pos(v.P, prevBone)).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
