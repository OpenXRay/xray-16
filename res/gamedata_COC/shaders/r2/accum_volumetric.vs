#include "common.h"

float3	vMinBounds;
float3	vMaxBounds;

struct vf
{
	float4 	hpos		: POSITION;
	float3 	lightToPos	: TEXCOORD0;		// light center to plane vector
	float3 	vPos		: TEXCOORD1;		// position in camera space
	half 	fDensity	: TEXCOORD2;		// plane density alon Z axis
//	half2	tNoise 		: TEXCOORD3;		// projective noise
};

vf main (v_static v)
{
	vf 		o;
	float4	vPos;
	vPos.xyz 	= lerp( vMinBounds, vMaxBounds, v.P);	//	Position in camera space
	vPos.w 		= 1;
	o.hpos 		= mul			(m_P, vPos);		// xform, input in camera coordinates

	o.lightToPos = vPos.xyz - Ldynamic_pos.xyz;
	o.vPos = vPos;

//	o.fDensity = (vMaxBounds.z-vMinBounds.z)/2000.0h;
//	o.fDensity = (vMaxBounds.z-vMinBounds.z)/2000.0h*2;
	o.fDensity = 1.0h/40.0h;
//	o.fDensity = 1.0h/20.0h;

	return o;
}
