#include "common.h"

struct av 
{
	float4 	pos	: POSITION;	// (float,float,float,1)
	float4 	nc	: NORMAL;	// (float,float,float,clr)
	float4 	misc	: TEXCOORD0;	// (u(Q),v(Q),frac,???)
};

struct vf
{
	float4 HPOS	: POSITION;
	float3 COL0	: COLOR0;
	float2 TEX0	: TEXCOORD0;
	float  fog	: FOG;
};

uniform float3x4	m_xform;
uniform float4 		consts;		// {1/quant,1/quant,???,???}
uniform float4 		wave; 		// cx,cy,cz,tm
uniform float4 		wind; 		// direction2D
uniform float4		c_bias;		// + color
uniform float4		c_scale;	// * color
uniform float2 		c_sun;		// x=*, y=+

vf main (av v)
{
	vf 		o;

	// Transform to world coords
	float3 	pos	= mul	(m_xform, v.pos);

	// 
	float 	base 	= m_xform._24;			// take base height from matrix
	float 	dp	= calc_cyclic  (wave.w+dot(pos,(float3)wave));
	float 	H 	= pos.y - base;			// height of vertex (scaled, rotated, etc.)
	float 	frac 	= v.misc.z*consts.x;		// fractional (or rigidity)
	float 	inten 	= H * dp;			// intensity
	float2 	result	= calc_xz_wave	(wind.xz*inten, frac);
	float4 	f_pos 	= float4(pos,1);		//float4(pos.x+result.x, pos.y, pos.z+result.y, 1);

	// Calc fog
	o.fog 		= calc_fogging 	(f_pos);

	// Final xform
	o.HPOS		= mul		(m_VP, f_pos);

	// Lighting
	float3 	N 	= mul (m_xform,  unpack_normal(v.nc)); 	//normalize 	(mul (m_xform,  unpack_normal(v.nc)));
	float 	L_base 	= v.nc.w;								// base hemisphere
	float4 	L_unpack= c_scale*L_base+c_bias;						// unpacked and decompressed
	float3 	L_rgb 	= L_unpack.xyz	;
	float3 	L_hemi 	= v_hemi_wrap(N,.75f)* L_unpack.w;					// hemisphere
	float3 	L_sun 	= v_sun_wrap (N,.25f)* (L_base*c_sun.x+c_sun.y);			// sun
	float3 	L_final	= L_rgb + L_hemi + L_sun + L_ambient;
	o.COL0		= L_final;	//,1);

	// final xform, color, tc
	o.TEX0.xy	= (v.misc * consts).xy;

	return o;
}
