#include "common.h"
#include "skin.h"

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;	// base
	float4 tc1	: TEXCOORD1;	// projected lmap
#ifdef T_DETAILS
	float2 tcd	: TEXCOORD2;	// details
#endif
	float3 c0	: COLOR0;		// sun-color
	float4 c1	: COLOR1;		// xyz=lq-color, w=factor
	float  fog	: FOG;
};

vf _main (v_model v)
{
	vf 		o;

	float4 pos 		= v.pos;
	float3 pos_w 	= mul(m_W, pos);
	float4 pos_w4 	= float4(pos_w,1);
	float3 norm_w 	= normalize(mul(m_W,v.norm));

	o.hpos 			= mul(m_WVP, pos);					// xform, input in world coords
	o.tc0			= v.tc.xy;							// copy tc
	o.tc1			= calc_model_lmap(pos_w);
	
#ifdef T_DETAILS
	o.tcd			= o.tc0.xy*dt_params;				// dt tc
#endif

	o.c0 			= calc_sun(norm_w); 				// sun-color
	o.c1 			= float4(calc_model_lq_lighting(norm_w), m_plmap_clamp[0].w);// lq-color
	o.fog 			= calc_fogging(pos_w4);				// fog, input in world coords

#ifdef SKIN_COLOR
	o.c1.rgb	   *= v.rgb_tint;
	o.c1.w			= 1;
#endif

	return o;
}

/////////////////////////////////////////////////////////////////////////
#define SKIN_VF vf
#include "skin_main.h"