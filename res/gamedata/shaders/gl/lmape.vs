#define	USE_LM_HEMI
#include "common.h"
#include "iostructs\v_lmape.h"

v2p _main(v_static v)
{
	v2p 		o;

	float3 	pos_w	= v.P.xyz;
	float3 	norm_w	= normalize(unpack_normal(v.Nh));
	
	o.hpos 		= mul			(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.tc,v.T.w,v.B.w);		// copy tc
	o.tc1		= unpack_tc_lmap	(v.lmh);			// copy tc 
	o.tc2 		= o.tc1;
	o.tc3		= calc_reflection	(pos_w, norm_w);
	o.c0		= v_hemi(norm_w);					// just hemisphere
	o.c1 		= v_sun	(norm_w);  					// sun
	o.fog	 	= saturate(calc_fogging (v.P));				// fog, input in world coords

	return o;
}
