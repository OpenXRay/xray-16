#include "common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"

struct vf
{
	float4 hpos	: POSITION	;
	float2 tbase	: TEXCOORD0	;
	float3 tenv	: TEXCOORD1	;
	float4 c0	: COLOR0	;	// c0=all lighting, c0.a = refl amount
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	float4 	P 	= v.P;
	float3 	N 	= unpack_normal	(v.N);
		
		P 	= watermove	(P);

	float2 	tc_base	= unpack_tc_base	(v.uv0,v.T.w,v.B.w);		// copy tc

	float 	amount	;
	float3 	tc_refl	= waterrefl 		(amount, P,N);

	o.tbase		= tc_base;
	o.tenv		= tc_refl;

	float3 	L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float3 	L_sun 	= v_sun(N)*v.color.w;					// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.hpos 		= mul			(m_VP, P);			// xform, input in world coords
	o.c0		= float4		(L_final,amount);
	o.fog 		= calc_fogging 		(P);				// fog, input in world coords

	return o;
}
