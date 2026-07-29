#ifndef SKINNED_VELOCITY_COMMON_H
#define SKINNED_VELOCITY_COMMON_H

StructuredBuffer<float4x4> g_BoneMatrices : register(t3);
StructuredBuffer<float4x4> g_PrevBoneMatrices : register(t4);

cbuffer VelocityTransforms : register(b0)
{
    float4x4 vel_m_W;
    float4x4 vel_m_W_prev;
    float4x4 vel_m_VP;
    float4x4 vel_m_VP_prev;
};

cbuffer VelocitySkinCB : register(b4)
{
    uint g_VelBoneOffset;
    uint g_VelPrevBoneOffset;
    uint g_VelHasPrev;
    uint g_VelPad;
};

float4 skinning_pos(float4 pos, float4x4 bone)
{
    return mul(bone, pos);
}

float4x4 get_curr_bone(int legacy_index)
{
    return g_BoneMatrices[g_VelBoneOffset + (legacy_index / 3)];
}

float4x4 get_prev_bone(int legacy_index)
{
    if (g_VelHasPrev)
        return g_PrevBoneMatrices[g_VelPrevBoneOffset + (legacy_index / 3)];
    return g_BoneMatrices[g_VelBoneOffset + (legacy_index / 3)];
}

float2 VelocityUvFromClip(float4 clip)
{
    float2 ndc = clip.xy / max(clip.w, 1e-5);
    ndc.y = -ndc.y;
    return ndc * 0.5 + 0.5;
}

struct VelocityVSOut
{
    float4 position : SV_Position;
    float4 currClip : TEXCOORD0;
    float4 prevClip : TEXCOORD1;
};

VelocityVSOut EmitVelocity(float3 currWorld, float3 prevWorld)
{
    VelocityVSOut o;
    o.currClip = mul(vel_m_VP, float4(currWorld, 1.0));
    o.prevClip = mul(vel_m_VP_prev, float4(prevWorld, 1.0));
    o.position = o.currClip;
    return o;
}

#endif
