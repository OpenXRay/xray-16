#include "common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"

struct vf
{
	float4 hpos	: POSITION	;
	float2 tbase	: TEXCOORD0	;
	float2 tdist0	: TEXCOORD1	;
	float2 tdist1	: TEXCOORD2	;
};

vf main (v_vert v)
{
	vf 		o;

	float4 	P 	= v.P;
	float3 	N 	= unpack_normal		(v.N);
		P 	= watermove		(P);

	o.tbase		= unpack_tc_base	(v.uv0,v.T.w,v.B.w);		// copy tc
	o.tdist0	= watermove_tc 		(o.tbase*W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
	o.tdist1	= watermove_tc 		(o.tbase*W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);
	o.hpos 		= mul			(m_VP, P);			// xform, input in world coords

	return o;
}
