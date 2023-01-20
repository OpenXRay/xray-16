#include "common.h"
#include "shared\wmark.h"

//	for multiplicative decal

//	For alpha-blend decal
struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float4 c0	: COLOR0;		// c0=all lighting
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	float3 	N 	= 	unpack_normal	(v.N);
	float4 	P 	= 	wmark_shift		(v.P,N);
	o.hpos 		= 	mul				(m_VP, P);				// xform, input in world coords
	o.tc0		= 	unpack_tc_base	(v.uv0,v.T.w,v.B.w);		// copy tc

	float3 		L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 		L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float3 		L_sun 	= v_sun(N)*v.color.w;				// sun
	float3 		L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.c0.rgb		= 	L_final;
	o.c0.a 		= 	calc_fogging 		(P);				// fog, input in world coords
	o.fog 		= 	1;				// fog, input in world coords

	return o;
}

/*
//	For alpha-blend decal
struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float3 c0	: COLOR0;		// c0=all lighting
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	float3 	N 	= 	unpack_normal	(v.N);
	float4 	P 	= 	wmark_shift		(v.P,N);
	o.hpos 		= 	mul				(m_VP, P);				// xform, input in world coords
	o.tc0		= 	unpack_tc_base	(v.uv,v.T.w,v.B.w);		// copy tc

	float3 		L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 		L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float3 		L_sun 	= v_sun(N)*v.color.w;				// sun
	float3 		L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.c0		= 	L_final;
	o.fog 		= 	calc_fogging 		(P);				// fog, input in world coords

	return o;
}
*/