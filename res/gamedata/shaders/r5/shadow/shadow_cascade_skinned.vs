// shadow_cascade_skinned.vs — 1-bone HUD/actor caster into sun CSM
#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT
{
	float4 P  : POSITION;
	float4 N  : NORMAL;
	float4 T  : TANGENT;
	float4 B  : BINORMAL;
	float2 tc : TEXCOORD0;
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

float4 unpack_skinned_position(float4 v)
{
	return float4(v.xyz * 12.0, 1.0);
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 localPos = unpack_skinned_position(input.P);
	int boneIdx = int(input.N.w * 255.0 + 0.3);
	float4x4 bone = get_bone(boneIdx);
	float4 skinnedPos = skinning_pos(localPos, bone);
	float3 worldPos3 = mul(m_W, skinnedPos).xyz;
	output.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	output.texcoord = input.tc;
	output.materialID = g_SkinnedMaterialID;
	return output;
}
