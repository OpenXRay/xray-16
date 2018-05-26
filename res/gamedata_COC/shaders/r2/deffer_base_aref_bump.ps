#include "common.h"
#include "sload.h"

f_deffer 	main	( p_bumped I )
{
	f_deffer		O;

	surface_bumped 	S 		= sload 	(I);
	clip					(S.base.w-def_aref);

	// Sample normal, rotate it by matrix, encode position 
  	float3	Ne  = mul		(float3x3(I.M1, I.M2, I.M3), S.normal);
			Ne	= normalize	(Ne);

	// hemi,sun,material
	float 	ms	= xmaterial		;
#ifdef USE_LM_HEMI
	float4	lm 	= tex2D			(s_hemi, I.lmh);
//	float 	h  	= dot			(lm.rgb,1.h/3.h);
	float 	h  	= get_hemi(lm);
# ifdef USE_R2_STATIC_SUN
		 	//ms 	= lm.w;
			ms 	= get_sun(lm);
# endif
#else
	float 	h	= I.position.w	;
# ifdef USE_R2_STATIC_SUN
		 	ms	= I.tcdh.w		;
# endif
#endif

	O.Ne 		= float4		(Ne,										h);
  	O.position	= float4 	(I.position.xyz + Ne*S.height*def_virtualh, ms);
  	O.C			= float4		(S.base.x,S.base.y,S.base.z,				S.gloss);		// OUT: rgb.gloss

	return 	O	;
}
