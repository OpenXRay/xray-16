#include	"common.h"
#include	"skin.h"

#if defined(USE_PARALLAX) || defined(USE_STEEPPARALLAX)
	uniform float3x4	    m_invW;
#endif 	//	defined(USE_PARALLAX) || defined(USE_STEEPPARALLAX)

v2p_bumped _main( v_model I )
{
	float4	w_pos	= I.P;

	// Eye-space pos/normal
	v2p_bumped 	O;
	O.hpos 		= mul( m_WVP, w_pos	);
	float2 	tc 	= I.tc;
	float3	Pe	= mul( m_WV, w_pos );
	O.tcdh 		= float4( tc.xyyy );

	//  Hemi cube lighting
	float3	Nw	= mul		((float3x3)m_W, (float3)I.N);
	float3  hc_pos	= (float3)hemi_cube_pos_faces;
	float3	hc_neg	= (float3)hemi_cube_neg_faces;
	float3  hc_mixed= (Nw < 0) ? hc_neg : hc_pos;
	float	hemi_val= dot( hc_mixed, abs(Nw) );
	hemi_val	= saturate(hemi_val);

	O.position	= float4(Pe, 	hemi_val);		//Use L_material.x for old behaviour;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w	= L_material.y;					// (,,,dir-occlusion)
#endif

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	//		     = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	float3 	N 	= I.N;		// just scale (assume normal in the -.5f, .5f)
	float3 	T 	= I.T;		// 
	float3 	B 	= I.B;		// 
	float3x3 xform	= mul	((float3x3)m_WV, float3x3(
						2*T.x,2*B.x,2*N.x,
						2*T.y,2*B.y,2*N.y,
						2*T.z,2*B.z,2*N.z
				));
	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic? 

	// Feed this transform to pixel shader
	O.M1 			= xform	[0]; 
	O.M2 			= xform	[1]; 
	O.M3 			= xform	[2]; 

#ifdef 	USE_TDETAIL
	O.tcdbump		= O.tcdh * dt_params;		// dt tc
#endif

	return	O;
}

/////////////////////////////////////////////////////////////////////////
#ifdef 	SKIN_NONE
v2p_bumped	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
v2p_bumped	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); }
#endif

#ifdef	SKIN_1
v2p_bumped	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); }
#endif

#ifdef	SKIN_2
v2p_bumped	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); }
#endif

#ifdef	SKIN_3
v2p_bumped	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); }
#endif

#ifdef	SKIN_4
v2p_bumped	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); }
#endif

FXVS;
