#define SM_6_0


#include "skinned_velocity_common.h"

struct VS_INPUT
{
    float4 P   : POSITION;
    float4 N   : NORMAL;
    float4 T   : TANGENT;
    float4 B   : BINORMAL;
    float2 tc  : TEXCOORD0;
    float4 ind : BLENDINDICES;
};

VelocityVSOut main(VS_INPUT v)
{
    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = v.B.w;
    float w3 = 1.0 - w0 - w1 - w2;
    int id0 = int(v.ind.x * 255.0 + 0.3);
    int id1 = int(v.ind.y * 255.0 + 0.3);
    int id2 = int(v.ind.z * 255.0 + 0.3);
    int id3 = int(v.ind.w * 255.0 + 0.3);

    float4x4 bone = get_curr_bone(id0) * w0 + get_curr_bone(id1) * w1
                  + get_curr_bone(id2) * w2 + get_curr_bone(id3) * w3;
    float4x4 prevBone = get_prev_bone(id0) * w0 + get_prev_bone(id1) * w1
                      + get_prev_bone(id2) * w2 + get_prev_bone(id3) * w3;

    float3 currWorld = mul(vel_m_W, skinning_pos(v.P, bone)).xyz;
    float3 prevWorld = mul(vel_m_W_prev, skinning_pos(v.P, prevBone)).xyz;
    return EmitVelocity(currWorld, prevWorld);
}
