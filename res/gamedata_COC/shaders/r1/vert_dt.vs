#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;		// detail
	float4 c0	: COLOR0;		// c0=all lighting
	float4 c1	: COLOR1;		// ps_1_1 read ports
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	float3 	N 	= unpack_normal		(v.N);
	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.uv,v.T.w,v.B.w);		// copy tc
//	o.tc0		= unpack_tc_base	(v.tc);				// copy tc
	o.tc1		= o.tc0*dt_params;					// dt tc

	float3 	L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float3 	L_sun 	= v_sun(N)*v.color.w;					// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;

	float2	dt 	= calc_detail		(v.P);

	o.c0		= float4(L_final.x,L_final.y,L_final.z,dt.x);
	o.c1		= dt.y;							//
	o.fog 		= calc_fogging 		(v.P);			// fog, input in world coords

	return o;
}
