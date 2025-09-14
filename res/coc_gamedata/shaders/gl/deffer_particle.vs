#include 	"common.h"
#include	"iostructs\v_particle_flat.h"

v2p_particle _main( vv I )
{
	float4 	w_pos 	= I.P;

	// Eye-space pos/normal
	v2p_flat 		O;
	O.hpos 		= mul		(m_WVP,		w_pos	);
	O.N 		= normalize (eye_position-w_pos.xyz	);
	float3	Pe	= mul		(m_WV, 		I.P		);
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh 		= float4	(I.tc.xyyy			);
#else
	O.tcdh 		= float2	(I.tc.xyyy			);
#endif
	O.position	= float4	(Pe, 		0.2		);

#ifdef 	USE_TDETAIL
	O.tcdbump	= O.tcdh.xy * dt_params.xy;			// dt tc
#endif

	v2p_particle	pp;
	pp.color = I.c;
	pp.base = O;

	return		pp;
}
