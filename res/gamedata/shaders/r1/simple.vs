#include "common.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float  fog	: FOG;
};

vf main (v_vert v)
{
	vf 		o;

	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.uv0,v.T.w,v.B.w);		// copy tc
	o.fog 		= calc_fogging 		(v.P);				// fog, input in world coords

	return o;
}
