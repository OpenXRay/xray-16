#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float4 c0	: COLOR0;		// c0=all lighting
};

vf main (v_vert v)
{
	vf 		o;

	float3 	N 	= unpack_normal		(v.N);
	float3 	L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float 	L_sun 	= v.color.w;						// sun occl only
	float3 	L_final	= L_rgb + L_hemi + L_ambient;

	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.c0 		= float4 		(L_final.x,L_final.y,L_final.z,L_sun);
	return o;
}
