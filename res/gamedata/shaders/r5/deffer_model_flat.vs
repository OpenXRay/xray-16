#include "common.h"
#include "skin.h"

v2p_flat _main( v_model I )
{
	// world-space  N
	float3 	N_w	= mul( (float3x3)m_W, I.N.xyz );

	// Eye-space pos/normal
	v2p_flat 		O;
	float3	Pe	= mul( m_WV, I.P );
	O.hpos 		= mul( m_WVP, I.P );
	O.N 		= mul( (float3x3)m_WV, I.N.xyz );

	O.tcdh 		= I.tc.xy;

	//  Hemi cube lighting
	float3	Nw	= mul( (float3x3)m_W, I.N.xyz );
	float3  hc_pos	= hemi_cube_pos_faces.xyz;
	float3	hc_neg	= hemi_cube_neg_faces.xyz;
	float3  hc_mixed= select(hc_pos, hc_neg, Nw < 0);
	float	hemi_val= dot( hc_mixed, abs(Nw) );
	hemi_val	= saturate(hemi_val);

	O.position	= float4(Pe, 	hemi_val);		//Use L_material.x for old behaviour;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w	= L_material.y;							// (,,,dir-occlusion)
#endif

#ifdef USE_TDETAIL
	O.tcdbump	= O.tcdh*dt_params;					// dt tc
#endif

	return	O;
}

/////////////////////////////////////////////////////////////////////////
#ifdef 	SKIN_NONE
v2p_flat	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
v2p_flat	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); }
#endif

#ifdef	SKIN_1
v2p_flat	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); }
#endif

#ifdef	SKIN_2
v2p_flat	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); }
#endif

#ifdef	SKIN_3
v2p_flat	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); }
#endif

#ifdef	SKIN_4
v2p_flat	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); }
#endif

FXVS;
