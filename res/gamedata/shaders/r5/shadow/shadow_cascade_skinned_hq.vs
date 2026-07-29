// shadow_cascade_skinned_hq.vs — 1-bone HQ (36B) skinned caster into the sun CSM.
// Mirrors bindless_skinned_hq.vs vertex unpacking, but outputs light-space depth for
// the cascade (cb_LightVP) and only carries UV + materialID for the alpha-clip PS.
#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_1W
{
	float4 P  : POSITION;
	float4 N  : NORMAL;   // .w = bone index
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

VS_OUTPUT main(VS_INPUT_1W v)
{
	VS_OUTPUT o;
	int boneIdx = int(v.N.w * 255.0 + 0.3);
	float4x4 bone = get_bone(boneIdx);
	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(m_W, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc;
	o.materialID = g_SkinnedMaterialID;
	return o;
}
