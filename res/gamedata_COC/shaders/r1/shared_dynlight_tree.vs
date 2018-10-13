#include "common.h"

#ifdef DL_POINT
	#define _out vf_point
#else
	#define _out vf_spot
#endif

struct _in 
{
	float4 	P		: POSITION;		// (float,float,float,1)
	float4 	N		: NORMAL;		// (float,float,float,clr)
	float4 	misc	: TEXCOORD0;	// (u(Q),v(Q),frac,???)
};

uniform float3x4	m_xform;
uniform float4 		consts;			// {1/quant,1/quant,???,???}
#ifdef T_WAVE
uniform float4 		wave; 			// cx,cy,cz,tm
uniform float4 		wind; 			// direction2D
#endif

_out main (_in v)
{
	_out		o;

	float3 	N 	= normalize(mul (m_xform,  unpack_bx2(v.N)));
	// Transform to world coords
	float3 	_P	= mul(m_xform, v.P);

#ifdef T_WAVE
	float 	base 	= m_xform._24;			// take base height from matrix
	float 	dp		= calc_cyclic(wave.w+dot(_P, wave.xyz));
	float 	H 		= _P.y - base;			// height of vertex (scaled, rotated, etc.)
	float 	frac 	= v.misc.z*consts.x;	// fractional (or rigidity)
	float 	inten 	= H * dp;				// intensity
	float2 	result	= calc_xz_wave(wind.xz*inten, frac);
	float4 	P 		= float4(_P.x+result.x, _P.y, _P.z+result.y, 1.0f);
#else
	float4 	P 		= float4(_P, 1.0f);
#endif
	
	// Final xform
	o.hpos 		= mul			(m_VP, P);
	o.tc0		= v.misc.xy * consts.xy;
#ifdef DL_DETAILS
	float2 dt 	= calc_detail	(v.P);
	o.tcd		= o.tc0*dt_params;
#endif

#ifdef DL_POINT
	o.color	= calc_point(o.tc1,o.tc2,P,N);
#else
	o.color	= calc_spot (o.tc1,o.tc2,P,N);
#endif

#ifdef DL_DETAILS
	o.color = o.color * dt.x + dt.y;
#endif

	return o;
}