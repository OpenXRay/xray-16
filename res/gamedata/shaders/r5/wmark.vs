#include "common.h"
#include "shared\wmark.h"

struct vf
{
	float2 tc0	: TEXCOORD0;
	float3 c0	: COLOR0;
	float  fog	: FOG;
	float4 hpos	: SV_Position;
};

vf main (v_static v)
{
	vf 		o;

	float3 	N 	= 	unpack_normal	(v.Nh);
	float4 	P 	= 	wmark_shift	(v.P,N);
	o.hpos 		= 	mul		(m_VP, P);
	o.tc0		= 	unpack_tc_base	(v.tc,v.T.w,v.B.w);

	o.c0		= 0;
	o.fog 		= saturate(calc_fogging 		(v.P));

	return o;
}
