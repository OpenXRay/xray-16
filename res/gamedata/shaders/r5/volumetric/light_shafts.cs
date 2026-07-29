#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

cbuffer LightShaftParams : register(b6)
{
	float3 cb_ShaftSunDir;
	float  cb_ShaftIntensity;
	float3 cb_ShaftAlbedo;
	float  cb_ShaftDensityBoost;
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims) || cb_ShaftIntensity < 1e-4)
		return;

	float3 worldPos = FroxelToWorld(dtID);
	float3 toCam = normalize(cb_CameraPos - worldPos);
	float align = saturate(dot(toCam, normalize(cb_ShaftSunDir)));
	float boost = cb_ShaftDensityBoost * cb_ShaftIntensity * align * align;

	float4 accum = u_FroxelVolume[dtID];
	accum.rgb += cb_ShaftAlbedo * boost;
	accum.a += boost;
	u_FroxelVolume[dtID] = accum;
}
