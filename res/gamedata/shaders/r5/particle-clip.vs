#include "common.h"

struct vv
{
	float4 P	: POSITION;
	float2 tc	: TEXCOORD0;
	float4 c	: COLOR0;
};
struct v2p
{
	float2 tc	: TEXCOORD0;
	float4 c	: COLOR0;
	float4 hpos	: SV_Position;
};

v2p main (vv v)
{
	v2p 		o;

	o.hpos 		= mul	(m_WVP, v.P);		// xform, input in world coords
	o.hpos.z	= abs	(o.hpos.z);
	o.hpos.w	= abs	(o.hpos.w);
	o.tc		= v.tc;				// copy tc
	o.c		= v.c;				// copy color

	return o;
}
