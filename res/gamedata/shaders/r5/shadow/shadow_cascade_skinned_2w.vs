// shadow_cascade_skinned_2w.vs — 2-bone HQ (44B) skinned caster into the sun CSM.
// Mirrors bindless_skinned_2w.vs: lerp(bone_0, bone_1, N.w), indices in tc.zw.
#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_2W
{
	float4 P  : POSITION;
	float4 N  : NORMAL;    // .w = lerp weight
	float4 T  : TANGENT;
	float4 B  : BINORMAL;
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

VS_OUTPUT main(VS_INPUT_2W v)
{
	VS_OUTPUT o;
	int id_0 = int(v.tc.z);
	int id_1 = int(v.tc.w);
	float4x4 bone = lerp(get_bone(id_0), get_bone(id_1), v.N.w);
	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(m_W, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc.xy;
	o.materialID = g_SkinnedMaterialID;
	return o;
}
