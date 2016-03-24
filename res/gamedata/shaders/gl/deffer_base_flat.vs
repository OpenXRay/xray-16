#include "common.h"
#include "iostructs\v_static_flat.h"

v2p_flat _main ( v_in I )
{
	I.Nh			= unpack_D3DCOLOR(I.Nh);
	I.T				= unpack_D3DCOLOR(I.T);
	I.B				= unpack_D3DCOLOR(I.B);

	// Eye-space pos/normal
	v2p_flat 		O;
	float4	Pp 	= mul( m_WVP, I.P );
	O.hpos 		= Pp;
	O.N 		= mul( float3x3(m_WV), unpack_bx2(I.Nh) );
	float3	Pe	= mul( m_WV, I.P );

	float2	tc 	= unpack_tc_base( I.tc, I.T.w, I.B.w);	// copy tc
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh 		= float4( tc.xyyy );
#else
	O.tcdh 		= float2( tc.xyyy );
#endif
	O.position	= float4( Pe, I.Nh.w );

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	float 	s	= I.color.w	;							// (r,g,b,dir-occlusion)
	O.tcdh.w	= s;
#endif

#ifdef	USE_TDETAIL
	O.tcdbump	= O.tcdh.xy * dt_params.xy;					// dt tc
#endif

#ifdef	USE_LM_HEMI
	O.lmh 		= unpack_tc_lmap( I.lmh );
#endif

	return	O;
}
