#include "common.h"
#include "iostructs\v_wmark.h"
#include "shared\wmark.h"

v2p _main (v_static v)
{
	v2p 		o;

	float3 	N 	= unpack_normal	(v.Nh);
	float4 	P 	= wmark_shift	(v.P.xyz,N);
	o.hpos 		= mul		(m_VP, P);			// xform, input in world coords
	o.tc0		= unpack_tc_base(v.tc,v.T.w,v.B.w);		// copy tc

	//float3 	L_rgb 	= v.color.xyz;				// precalculated RGB lighting
	//float3 	L_hemi 	= v_hemi(N)*v.norm.w;			// hemisphere
	//float3 	L_sun 	= v_sun(N)*v.color.w;			// sun
	//float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.c0		= float3(0);	//L_final;
	o.fog 		= saturate	(calc_fogging(v.P));			// fog, input in world coords

	return o;
}
