#include "common.h"
#include "iostructs\v_particle.h"

v2p _main (vv v)
{
	v2p 		o;

	o.hpos 		= mul	(m_WVP, v.P);		// xform, input in world coords
	o.hpos.z	= abs	(o.hpos.z);
	o.hpos.w	= abs	(o.hpos.w);
	o.tc		= v.tc;				// copy tc
	o.c		= v.c;				// copy color

	return o;
}
