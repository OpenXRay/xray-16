#define SM_5_0
#include "common.h"

struct GPUObjectData
{
	float3 position;
	float radius;
	uint batchIndex;
	uint flags;
	float2 padding;
};

cbuffer LightShadowCullParams : register(b5)
{
	float4 g_FrustumPlanes[6];
	uint g_ObjectCount;
	float g_RadiusInflate;
	uint g_MaxOut;
	uint g_Pad;
};

StructuredBuffer<GPUObjectData> g_Objects : register(t0);
ByteAddressBuffer g_InputDrawArgs : register(t1);
StructuredBuffer<uint> g_InputMaterialIDs : register(t2);

RWStructuredBuffer<uint> g_OutIndices : register(u0);
RWByteAddressBuffer g_OutDrawArgs : register(u1);
RWStructuredBuffer<uint> g_OutMaterialIDs : register(u2);
RWByteAddressBuffer g_OutCount : register(u3);

bool LightFrustumTestSphere(float3 center, float radius, float4 planes[6])
{
	for (uint i = 0; i < 5; ++i)
	{
		float dist = dot(planes[i].xyz, center) + planes[i].w;
		if (dist > radius)
			return false;
	}
	return true;
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	uint objectIdx = dtID.x;
	if (objectIdx >= g_ObjectCount)
		return;

	GPUObjectData obj = g_Objects[objectIdx];
	float r = obj.radius * g_RadiusInflate;
	if (!LightFrustumTestSphere(obj.position, r, g_FrustumPlanes))
		return;

	uint slot;
	g_OutCount.InterlockedAdd(0, 1, slot);
	if (slot >= g_MaxOut)
		return;

	g_OutIndices[slot] = objectIdx;

	uint srcOff = objectIdx * 20;
	uint indexCount = g_InputDrawArgs.Load(srcOff);
	uint startIndex = g_InputDrawArgs.Load(srcOff + 8);
	int baseVertex = asint(g_InputDrawArgs.Load(srcOff + 12));

	uint dstOff = slot * 20;
	g_OutDrawArgs.Store(dstOff, indexCount);
	g_OutDrawArgs.Store(dstOff + 4, 1);
	g_OutDrawArgs.Store(dstOff + 8, startIndex);
	g_OutDrawArgs.Store(dstOff + 12, asuint(baseVertex));
	g_OutDrawArgs.Store(dstOff + 16, slot);

	g_OutMaterialIDs[slot] = g_InputMaterialIDs[objectIdx];
}
