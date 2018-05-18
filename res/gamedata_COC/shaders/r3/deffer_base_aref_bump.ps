#if ( defined(MSAA_ALPHATEST_DX10_1_ATOC) || defined(MSAA_ALPHATEST_DX10_1) ) 
#define EXTEND_F_DEFFER
#endif

#include "common.h"
#include "sload.h"

#ifdef	ATOC

float4 	main	( p_bumped I ) : SV_Target
{
	surface_bumped 	S 		= sload 	(I);
	S.base.w = (S.base.w-def_aref*0.5f)/(1-def_aref*0.5f);
	return S.base;
}

#else	//	ATOC

#ifdef	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	main	( p_bumped I, float4 pos2d : SV_Position )
#else	//	MSAA_ALPHATEST_DX10_1_ATOC
f_deffer 	main	( p_bumped I )
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC
{
	f_deffer		O;

#if !defined(MSAA_ALPHATEST_DX10_1)
	surface_bumped 	S 		= sload 	(I);

#if	!( defined(MSAA_ALPHATEST_DX10_1_ATOC) || defined(MSAA_ALPHATEST_DX10_0_ATOC) )
	clip					(S.base.w-def_aref);
#endif	//	!( defined(MSAA_ALPHATEST_DX10_1_ATOC) || defined(MSAA_ALPHATEST_DX10_1_ATOC) )
#ifdef 	MSAA_ALPHATEST_DX10_1_ATOC
	float alpha = (S.base.w-def_aref*0.5f)/(1-def_aref*0.5f);
	uint mask = alpha_to_coverage ( alpha, pos2d );
#endif	//	MSAA_ALPHATEST_DX10_1_ATOC

#else	//	!defined(MSAA_ALPHATEST_DX10_1)
	uint mask = 0x0;
	
	surface_bumped 	S 		= sload 	(I,MSAAOffsets[0]*(1.0/16.0));
	
	if( S.base.w-def_aref >= 0 ) mask |= 0x1;
	
	[unroll] for( int i = 1; i < MSAA_SAMPLES; ++i )
	{
		surface_bumped 	SI 		= sload 	(I,MSAAOffsets[i]*(1.0/16.0));
		if( SI.base.w-def_aref >= 0 ) mask |= ( uint(0x1) << i );
	}
	
	if( mask == 0x0 )
		discard;
#endif	//	!defined(MSAA_ALPHATEST_DX10_1)

	// Sample normal, rotate it by matrix, encode position 
  	float3	Ne  = mul		(float3x3(I.M1, I.M2, I.M3), S.normal);
			Ne	= normalize	(Ne);

	// hemi,sun,material
	float 	ms	= xmaterial		;
#ifdef USE_LM_HEMI
//	float4	lm 	= tex2D			(s_hemi, I.lmh);
	float4	lm 	= s_hemi.Sample( smp_rtlinear, I.lmh);
	//float 	h  	= dot			(lm.rgb,1.h/3.h);
	float 	h  	= get_hemi(lm);
# ifdef USE_R2_STATIC_SUN
//		 	ms 	= lm.w;
			ms 	= get_sun(lm);
# endif
#else
	float 	h	= I.position.w	;
# ifdef USE_R2_STATIC_SUN
		 	ms	= I.tcdh.w		;
# endif
#endif

#ifndef EXTEND_F_DEFFER
	O = pack_gbuffer(
						float4	(Ne,										h),
  						float4 	(I.position.xyz + Ne*S.height*def_virtualh, ms),
  						float4	(S.base.x,S.base.y,S.base.z,				S.gloss) );
#else
	O = pack_gbuffer(
						float4	(Ne,										h),
  						float4 	(I.position.xyz + Ne*S.height*def_virtualh, ms),
  						float4	( float3(S.base.x,S.base.y,S.base.z),		S.gloss),
  						mask );
#endif

	return 	O	;
}

#endif //	ATOC
