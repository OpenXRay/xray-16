// clear_volume.cs — zero froxel volume
#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims))
		return;
	u_FroxelVolume[dtID] = float4(0, 0, 0, 0);
}
