#include "common.h"
#include "iostructs\v_sky.h"

v2p _main (vi v)
{
	v2p		o;

	float4	tpos	    = mul	(1000, v.p);
        o.hpos              = mul       (m_WVP, tpos);						// xform, input in world coords, 1000 - magic number
	o.hpos.z	    = o.hpos.w;
	o.tc0			= v.tc0;                        					// copy tc
	o.tc1			= v.tc1;                        					// copy tc
//	float	scale	= tex2Dlod	(s_tonemap,float4(.5,.5,.5,.5)).x ;
//	float	scale	= s_tonemap.Load( int3(0,0,0) ).x;
//	float	scale	= s_tonemap.Load( int3(1,1,0) ).x;
	float	scale	= texelFetch( s_tonemap, int2(0,0), 0 ).x;
//	o.c				= float4	( v.c.rgb*(scale*1.7), v.c.a );      		// copy color, pre-scale by tonemap //float4 ( v.c.rgb*scale*2, v.c.a );
	o.c				= float4	( v.c.rgb*(scale*2.0), v.c.a );      		// copy color, pre-scale by tonemap //float4 ( v.c.rgb*scale*2, v.c.a );

	return	o;
}