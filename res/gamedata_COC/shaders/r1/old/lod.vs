#include "common.h"

struct vv
{
	float4 pos	: POSITION;
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;
	float4 c	: COLOR0;
	float4 f 	: COLOR1;
};
struct vf
{
	float4 	hpos	: POSITION;
 	half2 	tc0	: TEXCOORD0;	// base0
 	half2 	tc1	: TEXCOORD1;	// base1
 	half2 	tc2	: TEXCOORD2;	// hemi0
 	half2 	tc3	: TEXCOORD3;	// hemi1
	half4 	c	: COLOR0;	// color
	half4 	f	: COLOR1;	// color
	float  	fog	: FOG;
};

vf main (vv v)
{
	vf 		o;

	o.hpos 		= mul			(m_VP, v.pos);			// xform, input in world coords
	o.tc0		= v.tc0;						// copy tc
	o.tc1		= v.tc1;						// copy tc
	o.tc2		= v.tc0;
	o.tc3		= v.tc1;
	o.c		= v.c;
	o.f 		= v.f;
	o.fog 		= calc_fogging 		(v.pos);			// fog, input in world coords

	return o;
}
