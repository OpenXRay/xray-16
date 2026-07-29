#define SM_6_0


#include "skinned_velocity_common.h"

struct VS_INPUT
{
    float4 P  : POSITION;
    float4 N  : NORMAL;
    float4 T  : TANGENT;
    float4 B  : BINORMAL;
    float4 tc : TEXCOORD0;
};

VelocityVSOut main(VS_INPUT v)
{
    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    float w = v.N.w;
    float4x4 bone = lerp(get_curr_bone(id_0), get_curr_bone(id_1), w);
    float4x4 prevBone = lerp(get_prev_bone(id_0), get_prev_bone(id_1), w);

    float3 currWorld = mul(vel_m_W, skinning_pos(v.P, bone)).xyz;
    float3 prevWorld = mul(vel_m_W_prev, skinning_pos(v.P, prevBone)).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
