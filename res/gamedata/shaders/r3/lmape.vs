//	TODO: DX10: Check r2 path. If we always get hemi here
#define	USE_LM_HEMI
#include "common.h"

struct 	v_lmap
{
	float4 	pos		: POSITION;		// (float,float,float,1)
	float4	norm	: NORMAL;		// (nx,ny,nz,hemi occlusion)
	float2 	tc0		: TEXCOORD0;	// (base)
	float2	tc1		: TEXCOORD1;	// (lmap/compressed)
};
struct vf
{
	float2 tc0	: TEXCOORD0;
	float2 tc1	: TEXCOORD1;
	float2 tch	: TEXCOORD2;
	float3 tc2	: TEXCOORD3;
	float3 c0	: COLOR0;		// c0=hemi+v-lights, 	c0.a = dt*
	float3 c1	: COLOR1;		// c1=sun, 		c1.a = dt+
	float  fog	: FOG;
	float4 hpos	: SV_Position;
};

vf main(v_static v)
{
	vf 		o;

	float3 	pos_w	= v.P;
	float3 	norm_w	= normalize(unpack_normal(v.Nh));
	
	o.hpos 		= mul				(m_VP, v.P);			// xform, input in world coords
	o.tc0		= unpack_tc_base	(v.tc,v.T.w,v.B.w);		// copy tc
	o.tc1		= unpack_tc_lmap	(v.lmh);			// copy tc 
	o.tch 		= o.tc1;
	o.tc2		= calc_reflection	(pos_w, norm_w);
	o.c0		= v_hemi(norm_w);	// just hemisphere
	o.c1 		= v_sun	(norm_w);  	// sun
	o.fog.x 	= saturate(calc_fogging (v.P));			// fog, input in world coords

	return o;
}
