#include "common.h"
#include "iostructs\v_tree_shadow.h"

uniform float3x4		m_xform;
uniform float3x4		m_xform_v;
uniform float4 			consts; 	// {1/quant,1/quant,???,???}
uniform float4 			c_scale,c_bias,wind,wave;

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
#ifdef	USE_AREF
v2p_shadow_direct_aref _main ( v_shadow_direct_aref I )
#else	//	USE_AREF
v2p_shadow_direct _main ( v_shadow_direct I )
#endif	//	USE_AREF
{
#ifdef	USE_AREF
	v2p_shadow_direct_aref 	O;
#else	//	USE_AREF
	v2p_shadow_direct 		O;
#endif	//	USE_AREF
	

	// Transform to world coords
	float3 	pos	= mul		(m_xform , I.P);

	// 
	float 	base 	= m_xform[3][1];			// take base height from matrix
	float 	dp		= calc_cyclic  (wave.w+dot(pos,float3(wave)));
	float 	H 		= pos.y - base;			// height of vertex (scaled, rotated, etc.)
	float 	inten 	= H * dp;			// intensity
	float2 	result;
#ifdef	USE_TREEWAVE
			result	= float2(0);
#else	//	USE_TREEWAVE
#ifdef	USE_AREF
	float 	frac 	= I.tc.z*consts.x;		// fractional (or rigidity)
#else	//	USE_AREF
	float 	frac 	= 0;
#endif	//	USE_AREF
			result	= calc_xz_wave	(wind.xz*inten, frac);
#endif	//	USE_TREEWAVE

	float4 	f_pos 	= float4(pos.x+result.x, pos.y, pos.z+result.y, 1);

	O.hpos 	= mul		(m_VP,	f_pos	);
#ifdef	USE_AREF
	O.tc0 	= (I.tc * consts).xy;		//	+ result;
#endif	//	USE_AREF
#ifndef USE_HWSMAP
	O.depth = O.hpos.z;
#endif
 	return	O;
}
