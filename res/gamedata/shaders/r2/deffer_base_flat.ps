#include "common.h"
#include "sload.h"

f_deffer 	main	( p_flat I )
{
  f_deffer	O;

  // diffuse
  float3 D	= tbase		(I.tcdh);	// IN:  rgb.a

#ifdef	USE_TDETAIL
	D.rgb	= 2*D.rgb*tex2D	(s_detail, I.tcdbump).rgb;
#endif

	// hemi,sun,material
	float 	ms	= xmaterial		;
#ifdef USE_LM_HEMI
	float4	lm 	= tex2D			(s_hemi, I.lmh);
//	float 	h  	= dot			(lm.rgb,1.h/3.h);
	float 	h  	= get_hemi(lm);
# ifdef USE_R2_STATIC_SUN
//		 	ms 	= lm.w			;
			ms 	= get_sun(lm);
# endif
#else
	float 	h	= I.position.w	;
# ifdef USE_R2_STATIC_SUN
		 	ms	= I.tcdh.w		;
# endif
#endif

  // 2. Standart output
  O.Ne          = float4		(normalize((float3)I.N.xyz), 					h			);
  O.position    = float4 	(I.position.xyz + O.Ne.xyz*def_virtualh/2.h, 	ms			);
  O.C			= float4		(D.rgb,											def_gloss	);	// OUT: rgb.gloss

  return O;
}
