#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"

ByteAddressBuffer g_SrcVB : register(t20);
StructuredBuffer<uint4> g_Chunks : register(t21);
RWByteAddressBuffer g_DstVB : register(u0);

cbuffer PreskinParams : register(b5)
{
    uint g_ChunkBase;
    uint g_FormatID;
    uint g_Stride;
    uint g_PreskinPad;
};

#define PRESKIN_NONHQ 1u
#define PRESKIN_HQ1W 2u
#define PRESKIN_HQ4W 3u
#define PRESKIN_HQ2W 4u
#define PRESKIN_HQ3W 5u

float4 UnpackBGRA8(uint packed)
{
    return float4(float((packed >> 16) & 0xFFu), float((packed >> 8) & 0xFFu), float(packed & 0xFFu), float(packed >> 24)) / 255.0;
}

float3 UnpackSnorm16x3(uint2 packed)
{
    int3 s;
    s.x = int(packed.x << 16) >> 16;
    s.y = int(packed.x) >> 16;
    s.z = int(packed.y << 16) >> 16;
    return float3(s) / 32767.0;
}

float2 UnpackSnorm16x2(uint packed)
{
    int2 s;
    s.x = int(packed << 16) >> 16;
    s.y = int(packed) >> 16;
    return float2(s) / 32767.0;
}

uint PackDir(float3 d)
{
    uint3 u = uint3(clamp(d * 127.5 + 127.5, 0.0, 255.0));
    return (u.x << 16) | (u.y << 8) | u.z | 0xFF000000u;
}

int BoneIndex(float unormByte)
{
    return int(unormByte * 255.0 + 0.3);
}

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    uint4 chunk = g_Chunks[g_ChunkBase + gid.x];
    uint v = gtid.x;
    if (v >= chunk.w)
        return;

    SkinnedDrawRecord rec = g_SkinnedRecords[chunk.x];
    uint src = (chunk.y + v) * g_Stride;

    float4 localPos;
    float3 N;
    float3 T;
    float3 B;
    float2 uv;
    float4x4 bone;

    if (g_FormatID == PRESKIN_NONHQ)
    {
        uint4 w0 = g_SrcVB.Load4(src);
        uint2 w1 = g_SrcVB.Load2(src + 16u);
        localPos = float4(UnpackSnorm16x3(w0.xy) * 12.0, 1.0);
        float4 n = UnpackBGRA8(w0.z);
        float4 t = UnpackBGRA8(w0.w);
        float4 b = UnpackBGRA8(w1.x);
        N = n.xyz * 2.0 - 1.0;
        T = t.xyz * 2.0 - 1.0;
        B = b.xyz * 2.0 - 1.0;
        uv = UnpackSnorm16x2(w1.y);
        bone = mdi_get_bone(rec.boneOffset, BoneIndex(n.w));
    }
    else
    {
        localPos = asfloat(g_SrcVB.Load4(src));
        uint3 packed = g_SrcVB.Load3(src + 16u);
        float4 n = UnpackBGRA8(packed.x);
        float4 t = UnpackBGRA8(packed.y);
        float4 b = UnpackBGRA8(packed.z);
        N = n.xyz * 2.0 - 1.0;
        T = t.xyz * 2.0 - 1.0;
        B = b.xyz * 2.0 - 1.0;
        if (g_FormatID == PRESKIN_HQ1W)
        {
            uv = asfloat(g_SrcVB.Load2(src + 28u));
            bone = mdi_get_bone(rec.boneOffset, BoneIndex(n.w));
        }
        else if (g_FormatID == PRESKIN_HQ4W)
        {
            uv = asfloat(g_SrcVB.Load2(src + 28u));
            float4 ind = UnpackBGRA8(g_SrcVB.Load(src + 36u));
            float w0 = n.w;
            float w1 = t.w;
            float w2 = b.w;
            float w3 = 1.0 - w0 - w1 - w2;
            bone = mdi_get_bone(rec.boneOffset, BoneIndex(ind.x)) * w0
                 + mdi_get_bone(rec.boneOffset, BoneIndex(ind.y)) * w1
                 + mdi_get_bone(rec.boneOffset, BoneIndex(ind.z)) * w2
                 + mdi_get_bone(rec.boneOffset, BoneIndex(ind.w)) * w3;
        }
        else if (g_FormatID == PRESKIN_HQ2W)
        {
            float4 tc = asfloat(g_SrcVB.Load4(src + 28u));
            uv = tc.xy;
            float4x4 bone0 = mdi_get_bone(rec.boneOffset, int(tc.z));
            float4x4 bone1 = mdi_get_bone(rec.boneOffset, int(tc.w));
            bone = lerp(bone0, bone1, n.w);
        }
        else
        {
            float4 tc = asfloat(g_SrcVB.Load4(src + 28u));
            uv = tc.xy;
            float w0 = n.w;
            float w1 = t.w;
            float w2 = 1.0 - w0 - w1;
            bone = mdi_get_bone(rec.boneOffset, int(tc.z)) * w0
                 + mdi_get_bone(rec.boneOffset, int(tc.w)) * w1
                 + mdi_get_bone(rec.boneOffset, BoneIndex(b.w)) * w2;
        }
    }

    float4 skinnedPos = skinning_pos(localPos, bone);
    float3 skinnedN = skinning_dir(N, bone);
    float3 skinnedT = skinning_dir(T, bone);
    float3 skinnedB = skinning_dir(B, bone);
    float3x3 worldRot = (float3x3)rec.world;
    float3 worldPos = mul(rec.world, skinnedPos).xyz;
    float3 worldN = normalize(mul(worldRot, skinnedN));
    worldPos += mdi_apply_splat_deform(rec, worldPos, worldN);
    float3 worldT = normalize(mul(worldRot, skinnedT));
    float3 worldB = normalize(mul(worldRot, skinnedB));

    uint dst = (chunk.z + v) * 48u;
    g_DstVB.Store4(dst, uint4(asuint(worldPos), PackDir(worldN)));
    g_DstVB.Store4(dst + 16u, uint4(PackDir(worldT), PackDir(worldB), asuint(uv)));
    g_DstVB.Store4(dst + 32u, uint4(0u, 0u, 0xFFFFFFFFu, 0u));
}
