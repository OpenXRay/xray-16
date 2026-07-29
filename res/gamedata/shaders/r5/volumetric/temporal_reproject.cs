#include "volumetric/shared_froxel.h"

Texture3D<float4> t_PrevFroxel : register(t0);
RWTexture3D<float4> u_FroxelVolume : register(u0);
SamplerState smp_linear : register(s0);

cbuffer TemporalParams : register(b6)
{
	float4x4 cb_PrevVP;
	float    cb_TemporalAlpha;
	uint     cb_FrameIndex;
	float2   cb_PadT;
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims))
		return;

	float4 cur = u_FroxelVolume[dtID];
	float3 worldPos = FroxelToWorld(dtID);

	float4 clip = mul(cb_PrevVP, float4(worldPos, 1.0));
	float3 ndc = clip.xyz / max(clip.w, 1e-5);
	float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);

	float3 viewVec = worldPos - cb_CameraPos;
	float viewZ = max(length(viewVec), cb_Near);
	float zNorm = ViewZToFroxelZ(viewZ, cb_Dims.z) / max((float)(cb_Dims.z - 1), 1.0);
	float3 prevUVW = float3(uv, zNorm);

	float4 hist = cur;
	if (all(uv >= 0.0) && all(uv <= 1.0) && zNorm >= 0.0 && zNorm <= 1.0)
		hist = t_PrevFroxel.SampleLevel(smp_linear, prevUVW, 0);

	bool shouldUpdate = ((dtID.x + dtID.y + dtID.z + cb_FrameIndex) % 4u) == 0u;
	if (shouldUpdate)
	{
		float a = saturate(cb_TemporalAlpha);
		u_FroxelVolume[dtID] = lerp(cur, hist, a);
	}
	else
	{
		u_FroxelVolume[dtID] = hist;
	}
}
