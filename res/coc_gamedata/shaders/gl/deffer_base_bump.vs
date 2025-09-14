#include	"common.h"
#include	"iostructs\v_static_bump.h"

v2p_bumped _main( v_in I )
{
//	I.color.rgb 	= I.color.bgr;	//	Swizzle to compensate DX9/DX10 format mismatch
	float4	w_pos	= I.P				;
	float2 	tc		= unpack_tc_base	(I.tc,I.T.w,I.B.w);	// copy tc
	float 	hemi 	= I.Nh.w			;

	// Eye-space pos/normal
	v2p_bumped 	O;
	float3	Pe	= mul		(m_WV,  w_pos		);
	O.hpos 		= mul		(m_WVP,	w_pos		);
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh 		= float4	(tc.xyyy			);
#else
	O.tcdh 		= float2	(tc.xyyy			);
#endif
	O.position	= float4	(Pe, hemi			);
//	O.position	= float4	(O.hpos.xyz, hemi	);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w	= I.color.w;					// (r,g,b,dir-occlusion)
#endif

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	//		     = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	I.Nh			= unpack_D3DCOLOR(I.Nh);
	I.T				= unpack_D3DCOLOR(I.T);
	I.B				= unpack_D3DCOLOR(I.B);
	float3 	N 	= unpack_bx4(I.Nh);	// just scale (assume normal in the -0.5, 0.5)
	float3 	T 	= unpack_bx4(I.T);	// 
	float3 	B 	= unpack_bx4(I.B);	// 
	float3x3 xform	= mul	(float3x3(m_WV), float3x3(T,B,N));
	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic? 

	// Feed this transform to pixel shader
	O.M1 			= xform[0]; 
	O.M2 			= xform[1]; 
	O.M3 			= xform[2]; 

#ifdef 	USE_TDETAIL
	O.tcdbump		= O.tcdh.xy * dt_params.xy;		// dt tc
#endif

#ifdef	USE_LM_HEMI
	O.lmh 			= unpack_tc_lmap	(I.lmh);
#endif
	return	O;
}
