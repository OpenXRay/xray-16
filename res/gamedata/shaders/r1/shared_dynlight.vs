#include "common.h"

#ifdef DL_POINT
	#define _out vf_point
#else
	#define _out vf_spot
#endif

#ifdef DL_LMAP
	#define _in v_lmap
#else
	#define _in v_vert
#endif

_out main (_in v)
{
	_out		o;

	float3 	N 	= unpack_bx2	(v.N);
#ifndef DL_WMARK
	float4	P	= v.P;
#else
	float4 	P 	= wmark_shift	(v.P,N);
#endif
	o.hpos 		= mul			(m_VP, P);				// xform, input in world coords
	o.tc0		= unpack_tc_base(v.uv0,v.T.w,v.B.w);	// copy tc
#ifdef DL_DETAILS
	float2 dt 	= calc_detail	(v.P);
	o.tcd		= o.tc0*dt_params;
#endif

#ifdef DL_POINT
	o.color		= calc_point(o.tc1,o.tc2,P,N);
#else
	o.color		= calc_spot (o.tc1,o.tc2,P,N);
#endif

#ifdef DL_DETAILS
	o.color 	= o.color * dt.x + dt.y;
#endif

	return o;
}