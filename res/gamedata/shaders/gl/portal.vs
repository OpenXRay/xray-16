#include "common.h"
#include "iostructs\v_portal.h"

v2p _main (v_vert v)
{
	v2p 		o;

	o.hpos		= mul(m_VP, v.pos);				// xform, input in world coords
	o.c			= v.color;
	o.fog 		= calc_fogging(v.pos);			// fog, input in world coords
	o.fog 		= saturate(o.fog);
	o.c.rgb 	= lerp(fog_color.rgb, o.c.rgb, o.fog);

//	float scale = tex2Dlod	(s_tonemap,float4(0.5,0.5,0.5,0.5)).x ;
//	float scale = s_tonemap.Load(int3(0,0,0)).x;
//	float scale = s_tonemap.Load(int3(1,1,0)).x;
	float scale = texelFetch(s_tonemap,int2(0,0),0).x;
	o.c.rgb		= o.c.rgb*scale;      		// copy color, pre-scale by tonemap //float4 ( v.c.rgb*scale*2, v.c.a );
	o.c.a		= o.fog;

	return o;
	
}
