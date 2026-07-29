#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"

struct VS_INPUT_2W
{
	float4 P  : POSITION;
	float4 N  : NORMAL;
	float4 T  : TANGENT;
	float4 B  : BINORMAL;
	float4 tc : TEXCOORD0;
	uint drawIndex : DRAWINDEX;
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
	SkinnedDrawRecord rec = skinned_mdi_record(v.drawIndex);
	int id_0 = int(v.tc.z);
	int id_1 = int(v.tc.w);
	float4x4 bone = lerp(mdi_get_bone(rec.boneOffset, id_0), mdi_get_bone(rec.boneOffset, id_1), v.N.w);
	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(rec.world, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc.xy;
	o.materialID = skinned_mdi_material(v.drawIndex);
	return o;
}
