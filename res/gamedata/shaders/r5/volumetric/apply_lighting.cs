// apply_lighting.cs — sun lighting of froxel density (unshadowed MVP)
#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

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
	// Simple isotropic phase; sun intensity scales inscattering
	float3 inscattered = albedo * density * cb_SunColor * cb_SunIntensity;
	// Store inscattered RGB + extinction (density) in A for ray march
	u_FroxelVolume[dtID] = float4(inscattered, density);
}
