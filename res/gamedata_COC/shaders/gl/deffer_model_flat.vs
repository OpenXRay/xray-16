#include "common.h"
#include "skin.h"
#include "iostructs\v_model_flat.h"

v2p_flat _main( v_model I )
{
	// world-space  N
	float3 	N_w	= mul( m_W, I.N );

	// Eye-space pos/normal
	v2p_flat 		O;
	float3	Pe	= mul( m_WV, I.P );
	O.hpos 		= mul( m_WVP, I.P );
	O.N 		= mul( float3x3(m_WV), float3(I.N) );
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh 		= float4	(I.tc.xyyy			);
#else
	O.tcdh 		= float2	(I.tc.xyyy			);
#endif
	
	//  Hemi cube lighting
	float3	Nw	= mul		(float3x3(m_W), float3(I.N));
	float3  hc_pos	= float3(hemi_cube_pos_faces);
	float3	hc_neg	= float3(hemi_cube_neg_faces);
	float3  hc_mixed= mask(lessThan(Nw, float3(0)), hc_neg, hc_pos);
	float	hemi_val= dot( hc_mixed, abs(Nw) );
	hemi_val	= saturate(hemi_val);

	O.position	= float4(Pe, 	hemi_val);		//Use L_material.x for old behaviour;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w	= L_material.y;							// (,,,dir-occlusion)
#endif

#ifdef USE_TDETAIL
	O.tcdbump	= O.tcdh.xy*dt_params.xy;					// dt tc
#endif

	return	O;
}
