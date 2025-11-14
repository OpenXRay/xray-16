#include "common.h"

uniform float4 		consts;
uniform float4 		wave;
uniform float4 		dir2D; 
	uniform float4 		array[61*4];

v2p_flat 	main (v_detail v)
{
	v2p_flat 		O;
	int 	i 	= v.misc.w;
	float4  m0 	= array[i+0];
	float4  m1 	= array[i+1];
	float4  m2 	= array[i+2];
	float4  c0 	= array[i+3];

	float4 	pos;
 	pos.x 		= dot	(m0, v.pos);
 	pos.y 		= dot	(m1, v.pos);
 	pos.z 		= dot	(m2, v.pos);
	pos.w 		= 1;

	float 	base 	= m1.w;
	float 	dp	= calc_cyclic   (dot(pos,wave));
	float 	H 	= pos.y - base;
	float 	frac 	= v.misc.z*consts.x;
	float 	inten 	= H * dp;
	float2 	result	= calc_xz_wave	(dir2D.xz*inten,frac);
	pos		= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);

	float3 	norm;
		norm.x 	= pos.x - m0.w	;
		norm.y 	= pos.y - m1.w	+ .75f;
		norm.z	= pos.z - m2.w	;

	float4	Pp 	= mul		(m_WVP,	pos				);
	O.hpos 		= Pp;
	O.N 		= mul		(m_WV,  normalize(norm)	);
	float3	Pe	= mul		(m_WV,  pos				);
	O.tcdh 		= float4	((v.misc * consts).xyyy );

# if defined(USE_R2_STATIC_SUN)
	O.tcdh.w	= c0.x;
# endif
	O.position	= float4	(Pe, 		c0.w		);

	return O;
}
FXVS;
