#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;
	float2 tch	: TEXCOORD2;
	float3 c0	: COLOR0;		// c0=hemi+v-lights, 	c0.a = dt*
	float3 c1	: COLOR1;		// c1=sun, 		c1.a = dt+
	float  fog	: FOG;
};

vf main (v_lmap v)
{
	vf 		o;

	float3	N 	= unpack_normal		(v.N);
	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.uv0,v.T.w,v.B.w);		// copy tc
//	o.tc0		= unpack_tc_base	(v.tc0);			// copy tc
	o.tc1		= unpack_tc_lmap	(v.uv1);			// copy tc 
	o.tch 		= o.tc1;
	o.c0		= v_hemi(N);						// just hemisphere
	o.c1 		= v_sun(N);  						// sun
	o.fog 		= calc_fogging 		(v.P);				// fog, input in world coords

	return o;
}
