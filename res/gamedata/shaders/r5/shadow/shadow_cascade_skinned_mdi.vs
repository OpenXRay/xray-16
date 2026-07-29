#define SM_6_0
#include "common.h"
#include "bindless_common.h"
#include "skinned_mdi_common.h"

struct VS_INPUT
{
	float4 P  : POSITION;
	float4 N  : NORMAL;
	float4 T  : TANGENT;
	float4 B  : BINORMAL;
	float2 tc : TEXCOORD0;
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

VS_OUTPUT main(VS_INPUT v)
{
	VS_OUTPUT o;
	SkinnedDrawRecord rec = skinned_mdi_record(v.drawIndex);
	float4 localPos = float4(v.P.xyz * 12.0, 1.0);
	int boneIdx = int(v.N.w * 255.0 + 0.3);
	float4x4 bone = mdi_get_bone(rec.boneOffset, boneIdx);
	float4 skinnedPos = skinning_pos(localPos, bone);
	float3 worldPos3 = mul(rec.world, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc;
	o.materialID = skinned_mdi_material(v.drawIndex);
	return o;
}
