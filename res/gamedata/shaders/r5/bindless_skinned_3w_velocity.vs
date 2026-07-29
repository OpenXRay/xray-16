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
    int id_2 = int(v.B.w * 255.0 + 0.3);
    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = 1.0 - w0 - w1;

    float4x4 bone = get_curr_bone(id_0) * w0 + get_curr_bone(id_1) * w1 + get_curr_bone(id_2) * w2;
    float4x4 prevBone = get_prev_bone(id_0) * w0 + get_prev_bone(id_1) * w1 + get_prev_bone(id_2) * w2;

    float3 currWorld = mul(vel_m_W, skinning_pos(v.P, bone)).xyz;
    float3 prevWorld = mul(vel_m_W_prev, skinning_pos(v.P, prevBone)).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
