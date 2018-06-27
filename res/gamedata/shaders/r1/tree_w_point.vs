#include "common.h"

struct av 
{
	float4 	pos	: POSITION;	// (float,float,float,1)
	float4 	nc	: NORMAL;	// (float,float,float,clr)
	float4 	misc	: TEXCOORD0;	// (u(Q),v(Q),frac,???)
};

uniform float3x4	m_xform;
uniform float4 		consts;		// {1/quant,1/quant,???,???}
uniform float4 		wave; 		// cx,cy,cz,tm
uniform float4 		wind; 		// direction2D
uniform float4		c_bias;		// + color
uniform float4		c_scale;	// * color
uniform float2 		c_sun;		// x=*, y=+

vf_point main (av v)
{
	vf_point	o;

	// Transform to world coords
	float3 	pos	= mul	(m_xform, v.pos);

	// 
	float 	base 	= m_xform._24;			// take base height from matrix
	float 	dp	= calc_cyclic  (wave.w+dot(pos,(float3)wave));
	float 	H 	= pos.y - base;			// height of vertex (scaled, rotated, etc.)
	float 	frac 	= v.misc.z*consts.x;		// fractional (or rigidity)
	float 	inten 	= H * dp;			// intensity
	float2 	result	= calc_xz_wave	(wind.xz*inten, frac);
	float4 	f_pos 	= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);
	float3 	f_N 	= normalize 	(mul (m_xform,  unpack_normal(v.nc)));

	// Final xform
	o.hpos		= mul		(m_VP, f_pos);
	o.tc0		= (v.misc * consts).xy;
	o.color		= calc_point 	(o.tc1,o.tc2,f_pos,f_N);

	return o;
}
