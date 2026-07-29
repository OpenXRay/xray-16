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
    float4 P   : POSITION;
    float4 N   : NORMAL;
    float4 T   : TANGENT;
    float4 B   : BINORMAL;
    float2 tc  : TEXCOORD0;
    float4 ind : BLENDINDICES;
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
    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = v.B.w;
    float w3 = 1.0 - w0 - w1 - w2;
    int id0 = int(v.ind.x * 255.0 + 0.3);
    int id1 = int(v.ind.y * 255.0 + 0.3);
    int id2 = int(v.ind.z * 255.0 + 0.3);
    int id3 = int(v.ind.w * 255.0 + 0.3);

    float4x4 bone = mdi_curr_bone(rec.boneOffset, id0) * w0
                  + mdi_curr_bone(rec.boneOffset, id1) * w1
                  + mdi_curr_bone(rec.boneOffset, id2) * w2
                  + mdi_curr_bone(rec.boneOffset, id3) * w3;
    float4x4 prevBone = mdi_prev_bone(rec, id0) * w0
                      + mdi_prev_bone(rec, id1) * w1
                      + mdi_prev_bone(rec, id2) * w2
                      + mdi_prev_bone(rec, id3) * w3;

    float3 currWorld = mul(rec.world, skinning_pos(v.P, bone)).xyz;
    float4x4 prevW = rec.prevValid ? rec.prevWorld : rec.world;
    float3 prevWorld = mul(prevW, skinning_pos(v.P, prevBone)).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
