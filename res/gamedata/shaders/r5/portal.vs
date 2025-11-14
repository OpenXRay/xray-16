#include "common.h"

struct	v_vert
{
	float4 	pos	: POSITION;
	float4	color	: COLOR0;
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

	o.hpos		= mul(m_VP, v.pos);
	o.c			= v.color;
	o.fog 		= calc_fogging(v.pos);
	o.fog 		= saturate(o.fog);
	o.c.rgb 	= lerp(fog_color, o.c, o.fog);

	float scale = s_tonemap.Load(int3(0,0,0)).x;
	o.c.rgb		= o.c.rgb*scale;
	o.c.a		= o.fog;

	return o;
	
}
