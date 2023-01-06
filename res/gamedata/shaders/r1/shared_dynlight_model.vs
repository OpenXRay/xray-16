#include "common.h"
#include "skin.h"

#ifdef DL_POINT
	#define _out vf_point
#else
	#define _out vf_spot
#endif

_out _main (v_model v)
{
	_out		o;

	float4 	pos 	= v.pos;
	float4  pos_w4 	= float4(mul(m_W,pos),1);
	float3 	norm_w 	= normalize(mul(m_W,v.norm));

	o.hpos 			= mul(m_WVP, pos);						// xform, input in world coords
	o.tc0			= v.tc.xy;								// copy tc
#ifdef DL_POINT
	o.color			= calc_point(o.tc1,o.tc2,pos_w4,norm_w);
#else
	o.color			= calc_spot(o.tc1,o.tc2,pos_w4,norm_w);
#endif

#ifdef DL_DETAILS
	o.tcd			= o.tc0*dt_params;
#endif

	return o;
}

/////////////////////////////////////////////////////////////////////////
#define SKIN_VF _out
#include "skin_main.h"