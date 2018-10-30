#include "common.h"
#include "iostructs\v_vert.h"

v2p _main (v_static_color v)
{
	v2p 		o;

	float3 	N 	= unpack_normal		(v.Nh.xyz);
	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.Tex0		= unpack_tc_base	(v.tc,v.T.w,v.B.w);		// copy tc

	float3 	L_rgb 	= v.color.bgr;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.Nh.w;					// hemisphere
	float3 	L_sun 	= v_sun(N)*v.color.w;					// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient.rgb;

	o.c0		= L_final;
	o.fog 		= saturate(calc_fogging 		(v.P));		// fog, input in world coords

	return o;
}
