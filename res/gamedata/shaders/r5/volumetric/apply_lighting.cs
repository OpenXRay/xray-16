#define SM_6_0
#define CSM_SHADOW_FORWARD
#include "common.h"
#include "shared/shadow_sampling.h"
#include "shared/clustered_lighting.h"
#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

StructuredBuffer<GPULightData> g_LightData : register(t20);
StructuredBuffer<uint2> g_ClusterGrid : register(t21);
StructuredBuffer<uint> g_LightIndexList : register(t22);

float3 EvaluateClusteredLightsVolumetric(float3 worldPos, float linearDepth)
{
	uint numLights = (uint)cluster_params.w;
	if (numLights == 0)
		return 0;

	uint clusterIdx = GetClusterIndexFromWorld(worldPos, linearDepth);
	uint2 clusterData = g_ClusterGrid[clusterIdx];
	uint lightOffset = clusterData.x;
	uint lightCount = clusterData.y;

	float3 total = 0;
	for (uint i = 0; i < lightCount; i++)
	{
		uint lightIdx = g_LightIndexList[lightOffset + i];
		GPULightData light = g_LightData[lightIdx];
		float3 lightPos = light.positionAndInvRangeSq.xyz;
		float invRangeSq = light.positionAndInvRangeSq.w;
		float3 lightColor = light.colorAndRange.xyz;
		float lightType = light.spotParamsAndType.y;

		float3 toLight = lightPos - worldPos;
		float distSq = dot(toLight, toLight);
		float atten = PointLightAttenuation(distSq, invRangeSq);

		if (lightType > 0.5f)
		{
			float3 spotDir = light.directionAndSpotScale.xyz;
			float spotScale = light.directionAndSpotScale.w;
			float spotOffset = light.spotParamsAndType.x;
			atten *= SpotLightAttenuation(toLight, spotDir, spotScale, spotOffset);
		}

		total += lightColor * atten;
	}
	return total;
}

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims))
		return;

	float4 v = u_FroxelVolume[dtID];
	float density = v.a;
	if (density <= 1e-6)
	{
		u_FroxelVolume[dtID] = float4(0, 0, 0, 0);
		return;
	}

	float3 albedo = v.rgb / density;
	float3 worldPos = FroxelToWorld(dtID);
	float linearDepth = abs(mul(m_V, float4(worldPos, 1.0)).z);

	float shadow = SampleCSM_Volumetric(worldPos);
	float3 sunLight = cb_SunColor * cb_SunIntensity * shadow;
	float3 pointLight = EvaluateClusteredLightsVolumetric(worldPos, linearDepth);
	float3 inscattered = albedo * density * (sunLight + pointLight);

	u_FroxelVolume[dtID] = float4(inscattered, density);
}
