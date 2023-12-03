#include "common.h"

uniform float4 		consts; // {1/quant,1/quant,diffusescale,ambient}
uniform float4 		wave; 	// cx,cy,cz,tm
uniform float4 		dir2D; 
//uniform float4 		array	[200] : register(c12);

#if INSTANCED_DETAILS
Texture1D<float4> array;
#else
//tbuffer DetailsData
//{
	uniform float4 		array[61*4];
//}
#endif

#if INSTANCED_DETAILS
v2p_flat 	main (v_detail v, uint instance_id : SV_InstanceID)
#else
v2p_flat 	main (v_detail v)
#endif
{
	v2p_flat 		O;
	// index
#if INSTANCED_DETAILS
	int 	i 	= instance_id * 2;

	float4 a0 = array.Load(int2(i, 0), 0);
	float4 a1 = array.Load(int2(i, 0), 1);

	float4  m0 	= float4(a0.y,    0, -a0.x, a1.x);
	float4  m1 	= float4(   0, a1.w,     0, a1.y);
	float4  m2 	= float4(a0.x,    0,  a0.y, a1.z);
	float4  c0 	= a0.zzzw;
#else
	int 	i 	= v.misc.w;
	float4  m0 	= array[i+0];
	float4  m1 	= array[i+1];
	float4  m2 	= array[i+2];
	float4  c0 	= array[i+3];
#endif

	// Transform pos to world coords
	float4 	pos;
 	pos.x 		= dot	(m0, v.pos);
 	pos.y 		= dot	(m1, v.pos);
 	pos.z 		= dot	(m2, v.pos);
	pos.w 		= 1;

	// 
	float 	base 	= m1.w;
	float 	dp	= calc_cyclic   (dot(pos,wave));
	float 	H 	= pos.y - base;			// height of vertex (scaled)
	float 	frac 	= v.misc.z*consts.x;		// fractional
	float 	inten 	= H * dp;
	float2 	result	= calc_xz_wave	(dir2D.xz*inten,frac);
	pos		= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);

	// Normal in world coords
	float3 	norm;	//	= float3(0,1,0);
		norm.x 	= pos.x - m0.w	;
		norm.y 	= pos.y - m1.w	+ .75f;	// avoid zero
		norm.z	= pos.z - m2.w	;

	// Final out
	float4	Pp 	= mul		(m_WVP,	pos				);
	O.hpos 		= Pp;
	float3 orig		= mul		(m_WV,  normalize(norm)	);
	O.N = lerp(orig, mul((float3x3)m_WV,  v.pos), 0.25);
	
	float3	Pe	= mul		(m_WV,  pos				);
//	O.tcdh 		= float4	((v.misc * consts).xy	);
	O.tcdh 		= float4	((v.misc * consts).xyyy );

# if defined(USE_R2_STATIC_SUN)
	O.tcdh.w	= c0.x;								// (,,,dir-occlusion)
# endif
	O.position	= float4	(Pe, 		c0.w		);

	return O;
}
FXVS;
