#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
#ifdef T_DETAILS
	float2 tcd	: TEXCOORD1;	// detail
	float4 c0	: COLOR0;		// xyz=all lighting, w=dt*
	float4 c1	: COLOR1;		// dt+
#else
	float3 c0	: COLOR0;		// all lighting
#endif
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	float3 	N 	= unpack_normal(v.N);
	o.hpos 		= mul(m_VP, v.P);						// xform, input in world coords
	o.tc0		= unpack_tc_base(v.uv0,v.T.w,v.B.w);
	
	float3 	L_rgb 	= v.color.xyz;						// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi(N)*v.N.w;					// hemisphere
	float3 	L_sun 	= v_sun(N)*v.color.w;				// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;
#ifdef T_DETAILS
	float2 dt 	= calc_detail(v.P);
	o.tcd		= o.tc0*dt_params;						// dt tc
	o.c0		= float4(L_final,dt.x);					// xyz=all lighting, w=dt*
	o.c1		= dt.y;									// dt+
#else
	o.c0		= L_final;								// c0=all lighting
#endif
	o.fog 		= calc_fogging(v.P);					// fog, input in world coords

	return o;
}
