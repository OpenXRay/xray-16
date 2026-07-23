// shadow_cascade_skinned_3w.vs — 3-bone HQ (44B) skinned caster into the sun CSM.
// Mirrors bindless_skinned_3w.vs: w0=N.w, w1=T.w, w2=1-w0-w1; id2 packed in B.w.
#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_3W
{
	float4 P  : POSITION;
	float4 N  : NORMAL;    // .w = weight0
	float4 T  : TANGENT;   // .w = weight1
	float4 B  : BINORMAL;  // .w = bone index 2
	float4 tc : TEXCOORD0; // .xy = UV, .zw = bone indices 0,1
};

struct VS_OUTPUT
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	nointerpolation uint materialID : TEXCOORD1;
};

cbuffer ShadowCascadeCB : register(b5)
{
	float4x4 cb_LightVP;
};

VS_OUTPUT main(VS_INPUT_3W v)
{
	VS_OUTPUT o;
	int id_0 = int(v.tc.z);
	int id_1 = int(v.tc.w);
	int id_2 = int(v.B.w * 255.0 + 0.3);
	float w0 = v.N.w;
	float w1 = v.T.w;
	float w2 = 1.0 - w0 - w1;
	float4x4 bone = get_bone(id_0) * w0 + get_bone(id_1) * w1 + get_bone(id_2) * w2;
	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(m_W, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc.xy;
	o.materialID = g_SkinnedMaterialID;
	return o;
}
