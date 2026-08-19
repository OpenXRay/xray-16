// shadow_cascade.vs — depth-only CSM caster (bindless mega-buffer path)
#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct VS_INPUT
{
	float4 position : POSITION;
	float4 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 binormal : BINORMAL;
	float2 texcoord : TEXCOORD0;
	float2 texcoord1: TEXCOORD1;
	float4 color    : COLOR0;
	uint drawIndex  : DRAWINDEX;
};

struct VS_OUTPUT
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	nointerpolation uint materialID : TEXCOORD1;
};

struct InstanceData
{
	float4x4 world;
	uint materialID;
	uint flags;
	float pad0, pad1;
};

cbuffer ShadowCascadeCB : register(b5)
{
	float4x4 cb_LightVP;
};

StructuredBuffer<InstanceData> g_InstanceData : register(t14);
StructuredBuffer<uint> g_CompactBatchIndices : register(t15);
StructuredBuffer<uint> g_CompactMaterialIDs : register(t16);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	uint drawID = input.drawIndex;
	uint batchIndex = g_CompactBatchIndices[drawID];
	InstanceData instanceData = g_InstanceData[batchIndex];
	uint materialID = g_CompactMaterialIDs[drawID];

	float4 worldPos = mul(instanceData.world, float4(input.position.xyz, 1.0));
	output.position = mul(cb_LightVP, worldPos);
	output.texcoord = input.texcoord;
	output.materialID = materialID;
	return output;
}
