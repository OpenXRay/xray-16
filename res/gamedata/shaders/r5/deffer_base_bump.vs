#include	"common.h"

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
#define	v_in	v_static_color	
#else
#define	v_in	v_static
#endif


v2p_bumped main( v_in I )
{
	float4	w_pos	= I.P				;
	float2 	tc		= unpack_tc_base	(I.tc,I.T.w,I.B.w);
	float 	hemi 	= I.Nh.w			;

	v2p_bumped 	O;
	float3	Pe	= mul		(m_WV,  w_pos		);
	O.hpos 		= mul		(m_WVP,	w_pos		);
	O.tcdh 		= tc.xy;
	O.position	= float4	(Pe, hemi			);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w	= I.color.w;
#endif

	I.Nh			= unpack_D3DCOLOR(I.Nh);
	I.T				= unpack_D3DCOLOR(I.T);
	I.B				= unpack_D3DCOLOR(I.B);
	float3 	N 	= unpack_bx4(I.Nh.xyz);
	float3 	T 	= unpack_bx4(I.T.xyz);
	float3 	B 	= unpack_bx4(I.B.xyz);
	float3x3 xform	= mul	((float3x3)m_WV, float3x3(
						T.x,B.x,N.x,
						T.y,B.y,N.y,
						T.z,B.z,N.z
				));

	O.M1 			= xform[0]; 
	O.M2 			= xform[1]; 
	O.M3 			= xform[2]; 

#ifdef 	USE_TDETAIL
	O.tcdbump		= O.tcdh * dt_params.xy;
#endif

#ifdef	USE_LM_HEMI
	O.lmh 			= unpack_tc_lmap	(I.lmh);
#endif
	return	O;
}

FXVS;
