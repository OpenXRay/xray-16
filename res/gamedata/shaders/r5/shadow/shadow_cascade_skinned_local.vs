#define SM_6_0
#define LOCAL_SKIN_INSTANCE_SSBO
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
	uint drawId : DRAWINDEX;
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
	LocalSkinInstanceGPU inst = g_LocalSkinInstances[input.drawId];
	float4 localPos = unpack_skinned_position(input.P);
	int boneIdx = int(input.N.w * 255.0 + 0.3);
	float4x4 bone = get_bone_ofs(inst.boneOffset, boneIdx);
	float4 skinnedPos = skinning_pos(localPos, bone);
	float3 worldPos3 = mul(inst.world, skinnedPos).xyz;
	output.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	output.texcoord = input.tc;
	output.materialID = inst.materialID;
	return output;
}
