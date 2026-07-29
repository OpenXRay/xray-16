// shadow_cascade_skinned_4w.vs — 4-bone HUD/actor caster into sun CSM
#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_4W
{
	float4 P   : POSITION;
	float4 N   : NORMAL;
	float4 T   : TANGENT;
	float4 B   : BINORMAL;
	float2 tc  : TEXCOORD0;
	float4 ind : BLENDINDICES;
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

VS_OUTPUT main(VS_INPUT_4W v)
{
	VS_OUTPUT o;

	float w0 = v.N.w;
	float w1 = v.T.w;
	float w2 = v.B.w;
	float w3 = 1.0 - w0 - w1 - w2;

	int id0 = int(v.ind.x * 255.0 + 0.3);
	int id1 = int(v.ind.y * 255.0 + 0.3);
	int id2 = int(v.ind.z * 255.0 + 0.3);
	int id3 = int(v.ind.w * 255.0 + 0.3);

	float4x4 bone = get_bone(id0) * w0
	              + get_bone(id1) * w1
	              + get_bone(id2) * w2
	              + get_bone(id3) * w3;

	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(m_W, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc;
	o.materialID = g_SkinnedMaterialID;
	return o;
}
