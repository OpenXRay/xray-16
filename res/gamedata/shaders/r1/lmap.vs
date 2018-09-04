#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;
	float2 tch	: TEXCOORD2;
#ifdef T_DETAILS
	float2 tcd	: TEXCOORD3;
	float4 c0	: COLOR0;		// xyz=hemi+v-lights, w=dt*
	float4 c1	: COLOR1;		// xyz=sun, w=dt+
#else
	float3 c0	: COLOR0;		// c0=hemi+v-lights
	float3 c1	: COLOR1;		// c1=sun
#endif
	float  fog	: FOG;
};

vf main (v_lmap v)
{
	vf 		o;

	float3 N 	= unpack_normal(v.N);

	o.hpos 		= mul(m_VP, v.P);					// xform, input in world coords
	o.tc0		= unpack_tc_base(v.uv0,v.T.w,v.B.w);
	o.tc1		= unpack_tc_lmap(v.uv1);
	o.tch 		= o.tc1;
#ifdef T_DETAILS
	float2 dt 	= calc_detail(v.P);
	o.tcd		= o.tc0*dt_params;					// dt tc
	o.c0		= float4(v_hemi(N),dt.x);			// xyz=hemi+v-lights, w=dt*
	o.c1 		= float4(v_sun(N),dt.y);			// xyz=sun, w=dt+
#else
	o.c0		= v_hemi(N);						// hemi+v-lights
	o.c1 		= v_sun(N);							// sun
#endif
	o.fog 		= calc_fogging(v.P);				// fog, input in world coords

	return o;
}
