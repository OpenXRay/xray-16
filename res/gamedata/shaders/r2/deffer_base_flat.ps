#include "common.h"
#include "sload.h"

f_deffer 	main	( p_flat I )
{
  f_deffer	O;

  // diffuse
  half3 D	= tbase		(I.tcdh);	// IN:  rgb.a

#ifdef	USE_TDETAIL
	D.rgb	= 2*D.rgb*tex2D	(s_detail, I.tcdbump).rgb;
#endif

	// hemi,sun,material
	half 	ms	= xmaterial		;
#ifdef USE_LM_HEMI
	half4	lm 	= tex2D			(s_hemi, I.lmh);
//	half 	h  	= dot			(lm.rgb,1.h/3.h);
	half 	h  	= get_hemi(lm);
# ifdef USE_R2_STATIC_SUN
//		 	ms 	= lm.w			;
			ms 	= get_sun(lm);
# endif
#else
	half 	h	= I.position.w	;
# ifdef USE_R2_STATIC_SUN
		 	ms	= I.tcdh.w		;
# endif
#endif

  // 2. Standart output
  O.Ne          = half4		(normalize((half3)I.N.xyz), 					h			);
  O.position    = half4 	(I.position.xyz + O.Ne.xyz*def_virtualh/2.h, 	ms			);
  O.C			= half4		(D.rgb,											def_gloss	);	// OUT: rgb.gloss

  return O;
}
