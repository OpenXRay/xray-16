#include "common.h"
#include "skin.h"
#include "iostructs\v_model_distort.h"

vf _main (v_model v)
{
	vf	o;

	o.hpos		= mul		(m_WVP, v.P);		// xform, input in world coords
	o.tc0		= v.tc.xy;				// copy tc

	// calculate fade
	float3  dir_v	= normalize	(mul(m_WV,v.P));
	float3  norm_v	= normalize	(mul(m_WV,v.N));
	float   fade	= 0.9*abs	(dot(dir_v,norm_v));
	o.c0		= float4	(fade);

	return	o;
}
