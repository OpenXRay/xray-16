#include "fluid_common.h"

cbuffer DynOOBBData
{
	float3x4	WorldToLocal;
	float3x4	LocalToWorld;
	float4		MassCenter;
	float4		OOBBWorldAngularVelocity;
	float4		OOBBWorldTranslationVelocity;
}

v2g_fluidsim_dyn_aabb main( v_fluidsim input)
{
    v2g_fluidsim_dyn_aabb output = (v2g_fluidsim_dyn_aabb)0;

    output.pos = float4(input.position.x, input.position.y, input.position.z, 1.0);
    output.cell0 = float3(input.textureCoords0.x, input.textureCoords0.y, input.textureCoords0.z);

	output.velocity = 0;

	{
		output.velocity = OOBBWorldTranslationVelocity.xyz;

		float3	r = mul( LocalToWorld, float4(output.cell0,1) ) - MassCenter.xyz;

		float3 AngularVel = cross( OOBBWorldAngularVelocity.xyz, r );

		output.velocity += AngularVel;

		output.velocity = mul( WorldToLocal, output.velocity );
	}
	
	for (int i=0; i<3; ++i)
	{
		output.clip0[i] = dot( float4(output.cell0,1), OOBBClipPlane[i]);
		output.clip1[i] = dot( float4(output.cell0,1), OOBBClipPlane[i+3]);
	}

    return output;
}