#include "common.h"

struct vf
{
	float4 P: POSITION	;
	float4 C: COLOR0	;
};

uniform float4 		tfactor;
vf main (vf i)
{
	vf 		o;

	o.P 		= mul			(m_WVP, i.P);			// xform, input in world coords
	o.C 		= tfactor*i.C;

	return o;
}
