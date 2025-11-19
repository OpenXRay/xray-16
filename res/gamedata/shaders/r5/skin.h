#ifndef SKIN_H
#define SKIN_H

#include "common.h"

struct v_model_skinned_0
{
    float4 P    : POSITION;
    float3 N    : NORMAL;
    float3 T    : TANGENT;
    float3 B    : BINORMAL;
    float2 tc   : TEXCOORD0;
};
struct v_model_skinned_1
{
    float4 P    : POSITION; 
    float4 N    : NORMAL;
    float3 T    : TANGENT; 
    float3 B    : BINORMAL; 
    float2 tc   : TEXCOORD0; 
};
struct v_model_skinned_2
{
    float4 P    : POSITION; 
    float4 N    : NORMAL;
    float3 T    : TANGENT; 
    float3 B    : BINORMAL; 
    float4 tc   : TEXCOORD0;
};
struct v_model_skinned_3
{
    float4 P    : POSITION; 
    float4 N    : NORMAL;
    float4 T    : TANGENT;
    float4 B    : BINORMAL;
    float4 tc   : TEXCOORD0;
};
struct v_model_skinned_4
{
    float4 P    : POSITION; 
    float4 N    : NORMAL;
    float4 T    : TANGENT;
    float4 B    : BINORMAL;
    float2 tc   : TEXCOORD0; 
    float4 ind  : BLENDINDICES;
};

float4 u_position(float4 v) { return float4(v.xyz, 1.f); }

uniform float3x4 sbones_array[78];

float3x4 get_bone(int legacy_index)
{
    return sbones_array[legacy_index / 3];
}

float3 skinning_dir(float3 dir, float3x4 bone_matrix)
{
    float3 U = unpack_normal(dir);
    return mul((float3x3)bone_matrix, U);
}

float4 skinning_pos(float4 pos, float3x4 bone_matrix)
{
    float4 P = u_position(pos);
    return float4(mul(bone_matrix, P), 1.0);
}

v_model skinning_0(v_model_skinned_0 v)
{
    v.N = v.N.zyx;
    v.T = v.T.zyx;
    v.B = v.B.zyx;

    v_model o;
    o.P  = u_position(v.P);
    o.N  = unpack_normal(v.N);
    o.T  = unpack_normal(v.T);
    o.B  = unpack_normal(v.B);
    o.tc = v.tc;
    return o;
}

v_model skinning_1(v_model_skinned_1 v)
{
    v.N.xyz = v.N.zyx;
    v.T.xyz = v.T.zyx;
    v.B.xyz = v.B.zyx;

    int mid = int(v.N.w * 255.0 + 0.3);
    
    float3x4 bone = get_bone(mid);

    v_model o;
    o.P  = skinning_pos(v.P, bone);
    o.N  = skinning_dir(v.N.xyz, bone);
    o.T  = skinning_dir(v.T.xyz, bone);
    o.B  = skinning_dir(v.B.xyz, bone);
    o.tc = v.tc;
    return o;
}

v_model skinning_2(v_model_skinned_2 v)
{
    v.N.xyz = v.N.zyx;
    v.T.xyz = v.T.zyx;
    v.B.xyz = v.B.zyx;

    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    
    float3x4 bone_0 = get_bone(id_0);
    float3x4 bone_1 = get_bone(id_1);

    float w = v.N.w;
    float3x4 bone = lerp(bone_0, bone_1, w);

    v_model o;
    o.P  = skinning_pos(v.P, bone);
    o.N  = skinning_dir(v.N.xyz, bone);
    o.T  = skinning_dir(v.T.xyz, bone);
    o.B  = skinning_dir(v.B.xyz, bone);
    o.tc = v.tc.xy;
    return o;
}

v_model skinning_3(v_model_skinned_3 v)
{
    v.N.xyz = v.N.zyx;
    v.T.xyz = v.T.zyx;
    v.B.xyz = v.B.zyx;

    int id_0 = int(v.tc.z);
    int id_1 = int(v.tc.w);
    int id_2 = int(v.B.w * 255.0 + 0.3);

    float3x4 bone_0 = get_bone(id_0);
    float3x4 bone_1 = get_bone(id_1);
    float3x4 bone_2 = get_bone(id_2);

    float w0 = v.N.w;
    float w1 = v.T.w;
    float w2 = 1 - w0 - w1;
    
    float3x4 bone = bone_0 * w0 + bone_1 * w1 + bone_2 * w2;

    v_model o;
    o.P  = skinning_pos(v.P, bone);
    o.N  = skinning_dir(v.N.xyz, bone);
    o.T  = skinning_dir(v.T.xyz, bone);
    o.B  = skinning_dir(v.B.xyz, bone);
    o.tc = v.tc.xy;
#ifdef SKIN_COLOR
    o.rgb_tint = float3(2,0,0);
    if (id_0 == id_1) o.rgb_tint = float3(1,2,0);
#endif
    return o;
}

v_model skinning_4(v_model_skinned_4 v)
{
    v.N.xyz   = v.N.zyx;
    v.T.xyz   = v.T.zyx;
    v.B.xyz   = v.B.zyx;
    v.ind.xyz = v.ind.zyx;

    float w[4];
    w[0] = v.N.w;
    w[1] = v.T.w;
    w[2] = v.B.w;
    w[3] = 1 - w[0] - w[1] - w[2];

    int id0 = int(v.ind.x * 255.0 + 0.3);
    float3x4 bone = get_bone(id0) * w[0];

    [unroll]
    for (int i = 1; i < 4; ++i)
    {
        int id = int(v.ind[i] * 255.0 + 0.3);
        bone += get_bone(id) * w[i];
    }

    v_model o;
    o.P  = skinning_pos(v.P, bone);
    o.N  = skinning_dir(v.N.xyz, bone);
    o.T  = skinning_dir(v.T.xyz, bone);
    o.B  = skinning_dir(v.B.xyz, bone);
    o.tc = v.tc;

    return o;
}

#endif
