#include "common.h"
#include "screenspace_fog.h"

struct vf
{
	float2 tc0	: TEXCOORD0;
	float3 c0	: COLOR0;		// c0=all lighting
	float  fog	: FOG;
	float4 hpos	: SV_Position;
};


vf main (v_static_color v)
{
	vf 		o;

	float3 	N 	= unpack_normal		(v.Nh);
	o.hpos 		= mul				(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.tc,v.T.w,v.B.w);		// copy tc

	float3 	L_rgb 	= v.color.zyx;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.Nh.w;					// hemisphere
	float3 	L_sun 	= v_sun(N)*v.color.w;					// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	o.c0		= L_final;
	o.fog 		= saturate(calc_fogging 		(v.P));			// fog, input in world coords
	o.fog		= SSFX_FOGGING(1.0 - o.fog, v.P.y); // Add SSFX Fog

	return o;
}
