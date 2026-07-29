#define SM_6_0
#define LOCAL_SKIN_INSTANCE_SSBO
#include "common.h"
#include "bindless_common.h"
#include "skinned_common.h"

struct VS_INPUT_3W
{
	float4 P  : POSITION;
	float4 N  : NORMAL;
	float4 T  : TANGENT;
	float4 B  : BINORMAL;
	float4 tc : TEXCOORD0;
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

VS_OUTPUT main(VS_INPUT_3W v)
{
	VS_OUTPUT o;
	LocalSkinInstanceGPU inst = g_LocalSkinInstances[v.drawId];
	int id_0 = int(v.tc.z);
	int id_1 = int(v.tc.w);
	int id_2 = int(v.B.w * 255.0 + 0.3);
	float w0 = v.N.w;
	float w1 = v.T.w;
	float w2 = 1.0 - w0 - w1;
	float4x4 bone = get_bone_ofs(inst.boneOffset, id_0) * w0
	              + get_bone_ofs(inst.boneOffset, id_1) * w1
	              + get_bone_ofs(inst.boneOffset, id_2) * w2;
	float4 skinnedPos = skinning_pos(v.P, bone);
	float3 worldPos3 = mul(inst.world, skinnedPos).xyz;
	o.position = mul(cb_LightVP, float4(worldPos3, 1.0));
	o.texcoord = v.tc.xy;
	o.materialID = inst.materialID;
	return o;
}
