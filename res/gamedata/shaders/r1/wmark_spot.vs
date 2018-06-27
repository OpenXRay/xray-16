#include "common.h"
#include "shared\wmark.h"

vf_spot main 	(v_vert v)
{
	vf_spot 	o;

	float3 	N 	= 	unpack_normal	(v.N);
	float4 	P 	= 	wmark_shift		(v.P,N);
	o.hpos 		= 	mul				(m_VP, P);				// xform, input in world coords
	o.tc0		= 	unpack_tc_base	(v.uv,v.T.w,v.B.w);		// copy tc
	o.color		= 	calc_spot 		(o.tc1,o.tc2,P,N);		// just hemisphere

	return 		o;
}
