#include "common.h"
#include "iostructs\v_volumetric.h"

//cbuffer VolumetricLights
//{
uniform	float3	vMinBounds;
uniform	float3	vMaxBounds;
uniform	float4	FrustumClipPlane[6];
//}

v2p _main ( float3 P )
{
	v2p 		o;
	float4	vPos;
	vPos.xyz 	= lerp( vMinBounds, vMaxBounds, P);	//	Position in camera space
	vPos.w 		= 1;
	o.hpos 		= mul			(m_P, vPos);		// xform, input in camera coordinates

	o.lightToPos = vPos.xyz - Ldynamic_pos.xyz;
	o.vPos = vPos.xyz;

//	o.fDensity = (vMaxBounds.z-vMinBounds.z)/2000.0;
//	o.fDensity = (vMaxBounds.z-vMinBounds.z)/2000.0*2.0;
	o.fDensity = 1.0/40.0;
//	o.fDensity = 1.0/20.0;

	for (int i=0; i<3; ++i)
	{
		o.clip0[i] = dot( o.hpos, FrustumClipPlane[i]);
		o.clip1[i] = dot( o.hpos, FrustumClipPlane[i+3]);
	}

	return o;
}