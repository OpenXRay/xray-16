#include "common.h"

struct vi
{
        float4        p        : POSITION        	;
        float4        c        : COLOR0        		;
        float3        tc0      : TEXCOORD0        	;
        float3        tc1      : TEXCOORD1        	;
};

struct vf
{
        float4        hpos     : POSITION        	;
        float4        c        : COLOR0        		;
        float3        tc0      : TEXCOORD0        	;
        float3        tc1      : TEXCOORD1        	;
};

vf main (vi v)
{
        vf                 	o;

	float4	tpos	    = mul	(1000, v.p);
        o.hpos              = mul       (m_WVP, tpos);						// xform, input in world coords, 1000 - magic number
	o.hpos.z	    = o.hpos.w;
        o.tc0               = v.tc0;                        					// copy tc
        o.tc1               = v.tc1;                        					// copy tc
#ifdef USE_VTF
        float	scale		= tex2Dlod	(s_tonemap,float4(.5,.5,.5,.5)).x ;
        o.c                	= float4	( v.c.rgb*scale*2, v.c.a );      		// copy color, pre-scale by tonemap //float4 ( v.c.rgb*scale*2, v.c.a );
#else
        o.c                	= v.c       ;										// copy color, low precision
#endif
        return              o;
}
