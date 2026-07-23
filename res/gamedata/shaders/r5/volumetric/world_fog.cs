// world_fog.cs — height fog density injection into froxel volume
#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims))
		return;

	float3 worldPos = FroxelToWorld(dtID);
	float height = worldPos.y - cb_FogBaseHeight;
	float heightFactor = exp(-max(height, 0.0) * cb_FogHeightFalloff);
	float density = cb_FogDensity * heightFactor;

	float4 prev = u_FroxelVolume[dtID];
	// rgb = albedo * density (accum), a = density
	u_FroxelVolume[dtID] = float4(prev.rgb + cb_FogAlbedo * density, prev.a + density);
}
