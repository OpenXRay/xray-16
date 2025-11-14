#include "common.h"

uniform float3x4		m_xform;
uniform float3x4		m_xform_v;
uniform float4 			consts;
uniform float4 			c_scale,c_bias,wind,wave;

#ifdef	USE_AREF
v2p_shadow_direct_aref main ( v_shadow_direct_aref I )
#else
v2p_shadow_direct main ( v_shadow_direct I )
#endif
{
#ifdef	USE_AREF
	v2p_shadow_direct_aref 	O;
#else
	v2p_shadow_direct 		O;
#endif
	

	float3 	pos	= mul		(m_xform , I.P);

	float 	base 	= m_xform._24;
	float 	dp		= calc_cyclic  (wave.w+dot(pos,wave.xyz));
	float 	H 		= pos.y - base;
	float 	inten 	= H * dp;
	float2 	result;
#ifdef	USE_TREEWAVE
			result	= 0;
#else
#ifdef	USE_AREF
	float 	frac 	= I.tc.z*consts.x;
#else
	float 	frac 	= 0;
#endif
			result	= calc_xz_wave	(wind.xz*inten, frac);
#endif

	float4 	f_pos 	= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);

	O.hpos 	= mul		(m_VP,	f_pos	);
#ifdef	USE_AREF
	O.tc0 	= (I.tc * consts).xy;
#endif
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
FXVS;
