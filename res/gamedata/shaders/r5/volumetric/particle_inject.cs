// particle_inject.cs — splat particle batch centers into froxel density (smoke MVP)
#include "volumetric/shared_froxel.h"

RWTexture3D<float4> u_FroxelVolume : register(u0);

struct ParticleInjectPoint
{
	float3 position;
	float  radius;
	float3 albedo;
	float  density;
};

StructuredBuffer<ParticleInjectPoint> g_Particles : register(t0);

cbuffer ParticleInjectParams : register(b6)
{
	uint cb_ParticleCount;
	uint3 cb_PadP;
};

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID >= cb_Dims) || cb_ParticleCount == 0)
		return;

	float3 worldPos = FroxelToWorld(dtID);
	float4 accum = u_FroxelVolume[dtID];

	uint count = min(cb_ParticleCount, 256u);
	for (uint i = 0; i < count; ++i)
	{
		ParticleInjectPoint p = g_Particles[i];
		float r = max(p.radius, 0.25);
		float d = length(worldPos - p.position);
		float w = saturate(1.0 - d / r);
		w = w * w;
		float dens = p.density * w;
		accum.rgb += p.albedo * dens;
		accum.a += dens;
	}

	u_FroxelVolume[dtID] = accum;
}
