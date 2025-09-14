#include "common.h"

struct av 
{
	float4 	pos	: POSITION;		// (float,float,float,1)
	float4 	nc	: NORMAL;		// (float,float,float,clr)
	float4 	misc: TEXCOORD0;	// (u(Q),v(Q),frac,???)
};

struct vf
{
	float4 hpos	: POSITION;
	float2 tc0	: TEXCOORD0;
#ifdef T_DETAILS
	float2 tcd	: TEXCOORD1;	// detail
	float4 c0	: COLOR0;		// xyz=all lighting, w=dt*
	float4 c1	: COLOR1;		// dt+
#else
	float3 c0	: COLOR0;		// all lighting
#endif
	float  fog	: FOG;
};

uniform float3x4	m_xform;
uniform float4 		consts;		// 1/quant,1/quant,???,???
#ifdef TREE_WAVE
uniform float4 		wave; 		// cx,cy,cz,tm
uniform float4 		wind; 		// direction2D
#endif
uniform float4		c_bias;		// + color
uniform float4		c_scale;	// * color
uniform float2 		c_sun;		// x=*, y=+

vf main (av v)
{
	vf 		o;

	// Transform to world coords
	float3 	pos		= mul(m_xform, v.pos);

#ifdef TREE_WAVE
	// Calc waving
	float 	base 	= m_xform._24;				// take base height from matrix
	float 	dp		= calc_cyclic(wave.w+dot(pos,(float3)wave));
	float 	H 		= pos.y - base;				// height of vertex (scaled, rotated, etc.)
	float 	frac 	= v.misc.z*consts.x;		// fractional (or rigidity)
	float 	inten 	= H * dp;					// intensity
	float2 	result	= calc_xz_wave(wind.xz*inten, frac);
	float4 	f_pos 	= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);
#else
	float4 	f_pos 	= float4(pos,1);
#endif

	// Calc fog
	o.fog 			= calc_fogging(f_pos);

	// Calc lighting
	float3 	N 		= mul(m_xform, unpack_normal(v.nc));	// normalize(mul(m_xform, unpack_normal(v.nc)));
	float 	L_base 	= v.nc.w;								// base hemisphere
	float4 	L_unpack= c_scale*L_base+c_bias;				// unpacked and decompressed
	float3 	L_rgb 	= L_unpack.xyz;							// precalculated RGB lighting
	float3 	L_hemi 	= v_hemi_wrap(N,0.75f)*L_unpack.w;		// hemisphere
	float3 	L_sun 	= v_sun(N)*(L_base*c_sun.x+c_sun.y);	// sun
	// Some alternative sunlight calculating formulas (GSC)
	//float3 	L_sun 	= v_sun_wrap(N,0.25f)*(L_base*c_sun.x+c_sun.y);
	//float3 	L_sun 	= L_sun_color*(0.25f+0.75f*dot(N,-L_sun_dir_w))*(L_base*c_sun.x+c_sun.y);
	//float3 	L_sun 	= L_sun_color*max(0,(1+dot(N,-L_sun_dir_w))/2)*(L_base*c_sun.x+c_sun.y);
	float3 	L_final	= L_rgb + L_hemi + L_sun;

	// Final xform, color, tc
	o.hpos			= mul(m_VP, f_pos);
	o.tc0			= (v.misc * consts).xy;
#ifdef T_DETAILS
	o.tcd			= o.tc0*dt_params;				// dt tc
	float2 dt 		= calc_detail(f_pos);			
	o.c0			= float4(L_final,dt.x);			// xyz=all lighting, w=dt*
	o.c1			= dt.y;							// dt+
#else
	o.c0			= L_final;
#endif

	return o;
}
