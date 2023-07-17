#include "common.h"
#include "screenspace_fog.h"

struct	v_vert
{
	float4 	pos	: POSITION;	// (float,float,float,1)
	float4	color	: COLOR0;	// (r,g,b,dir-occlusion)
};
struct 	v2p
{
	float4 c	: COLOR0;
	float  fog	: FOG;
	float4 hpos	: SV_Position;
};

v2p main (v_vert v)
{
	v2p 		o;

	o.hpos		= mul(m_VP, v.pos); // xform, input in world coords

	float fog	= saturate(calc_fogging(v.pos)); // fog, input in world coords
	o.fog 		= SSFX_FOGGING(1.0 - fog, v.pos.y); // Add SSFX Fog

	o.c.rgb 	= lerp(fog_color, v.color, o.fog * o.fog);	// fog blending
	o.c.a		= o.fog; // Alpha

	return o;
	
}
FXVS;
