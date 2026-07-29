#define SM_6_0


#include "skinned_velocity_common.h"

struct VS_INPUT
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float2 tc : TEXCOORD0;
};

VelocityVSOut main(VS_INPUT input)
{
    int boneIdx = int(input.N.w * 255.0 + 0.3);

    float4 currSkinned = skinning_pos(input.P, get_curr_bone(boneIdx));
    float3 currWorld = mul(vel_m_W, currSkinned).xyz;

    float4 prevSkinned = skinning_pos(input.P, get_prev_bone(boneIdx));
    float3 prevWorld = mul(vel_m_W_prev, prevSkinned).xyz;

    return EmitVelocity(currWorld, prevWorld);
}
