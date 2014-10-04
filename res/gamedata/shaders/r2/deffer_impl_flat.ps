#include "common.h"
#include "sload.h"

#define	USE_4_DETAIL

#if defined(USE_TDETAIL) && defined(USE_4_DETAIL)
#	define	USE_4_BUMP
#endif

#ifdef USE_4_BUMP
f_deffer 	main	( p_bumped I 	)
#else
f_deffer 	main	( p_flat I 		)
#endif
{
	f_deffer	O;

 	// diffuse
	half4 D		= tbase		(I.tcdh);			// IN:  rgb.a
	half4 L 	= tex2D		(s_lmap, I.tcdh);

	half	G	= def_gloss	;

# ifdef	USE_TDETAIL
 # ifdef USE_4_DETAIL
	half4	mask= tex2D		(s_mask, I.tcdh);
	half 	mag	= dot 		(mask,1);
			mask= mask/mag	;

	half3	d_R	= tex2D		(s_dt_r, I.tcdbump)*mask.r;
	half3	d_G	= tex2D		(s_dt_g, I.tcdbump)*mask.g;
	half3	d_B	= tex2D		(s_dt_b, I.tcdbump)*mask.b;
	half3	d_A	= tex2D		(s_dt_a, I.tcdbump)*mask.a;
	half3	dt	= d_R+d_G+d_B+d_A;
		D.rgb	= 2*D.rgb*dt	 ;

  # ifdef USE_4_BUMP

 	 half4	n_Rt = tex2D	(s_dn_r, I.tcdbump).wzyx;
	 half4	n_Gt = tex2D	(s_dn_g, I.tcdbump).wzyx;
	 half4	n_Bt = tex2D	(s_dn_b, I.tcdbump).wzyx;
	 half4	n_At = tex2D	(s_dn_a, I.tcdbump).wzyx;
	 
 	 half3	n_R = (n_Rt-0.5)*mask.r; half g_R=n_Rt.w*mask.r;
	 half3	n_G = (n_Gt-0.5)*mask.g; half g_G=n_Gt.w*mask.g;
	 half3	n_B = (n_Bt-0.5)*mask.b; half g_B=n_Bt.w*mask.b;
	 half3	n_A = (n_At-0.5)*mask.a; half g_A=n_At.w*mask.a;

	 half3	mix		= 	n_R+n_G+n_B+n_A;
			mix.z	*=	0.5;		//. make bump twice as contrast (fake, remove me if possible)
	 half3	N		= 	mul     	(half3x3(I.M1, I.M2, I.M3), mix.xyz);

	 	 	G 		= 	g_R+g_G+g_B+g_A;
//			G 	= 0			;
//			G 	= mix.w		;		// gloss
  # else
	half3	N 	= I.N.xyz	;
  # endif

 # else
	D.rgb	= 2*D.rgb*tex2D	(s_detail, I.tcdbump).rgb;
 # endif
# else
	half3	N 	= I.N.xyz	;
# endif

	// hemi, sun, material
	half 	ms	= xmaterial	;
# ifdef USE_R2_STATIC_SUN
		 	ms 	= L.w		;
# endif

  // 2. Standart output
  O.Ne          = half4		(normalize(N), 									D.w			);	// hemi
  O.position    = half4 	(I.position.xyz + O.Ne.xyz*def_virtualh/2.h, 	ms			);	//
  O.C			= half4		(D.x,	D.y,	D.z,							G			);	// OUT: rgb.gloss

  return O;
}
