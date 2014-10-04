#include "common.h"
#include "sload.h"

f_deffer 	main	( p_bumped I )
{
  f_deffer	O;

  surface_bumped 	S 	= sload 	(I);

  // Sample normal, rotate it by matrix, encode position 
  half3 Ne				= mul       (half3x3(I.M1, I.M2, I.M3), S.normal);
		Ne				= normalize	(Ne);


	// hemi,sun,material
	half 	ms	= xmaterial		;
#ifdef USE_LM_HEMI
	half4	lm 	= tex2D			(s_hemi, I.lmh);
//	half 	h  	= dot			(lm.rgb,1.h/3.h);
	half 	h  	= get_hemi(lm);
# ifdef USE_R2_STATIC_SUN
		 	ms 	= lm.w;
			ms 	= get_sun(lm);
# endif
#else
	half 	h	= I.position.w	;
# ifdef USE_R2_STATIC_SUN
		 	ms	= I.tcdh.w		;
# endif
#endif

	O.Ne 		= half4		(Ne, 										h		);
	O.position  = half4 	(I.position.xyz + Ne*S.height*def_virtualh, ms		);
	O.C			= half4		(S.base.xyz, 								S.gloss	);							// OUT: rgb.gloss

  return O;
}
