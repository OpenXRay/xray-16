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
	float4 D		= tbase		(I.tcdh);			// IN:  rgb.a
	float4 L 	= tex2D		(s_lmap, I.tcdh);

	float	G	= def_gloss	;

# ifdef	USE_TDETAIL
 # ifdef USE_4_DETAIL
	float4	mask= tex2D		(s_mask, I.tcdh);
	float 	mag	= dot 		(mask,1);
			mask= mask/mag	;

	float3	d_R	= tex2D		(s_dt_r, I.tcdbump)*mask.r;
	float3	d_G	= tex2D		(s_dt_g, I.tcdbump)*mask.g;
	float3	d_B	= tex2D		(s_dt_b, I.tcdbump)*mask.b;
	float3	d_A	= tex2D		(s_dt_a, I.tcdbump)*mask.a;
	float3	dt	= d_R+d_G+d_B+d_A;
		D.rgb	= 2*D.rgb*dt	 ;

  # ifdef USE_4_BUMP

 	 float4	n_Rt = tex2D	(s_dn_r, I.tcdbump).wzyx;
	 float4	n_Gt = tex2D	(s_dn_g, I.tcdbump).wzyx;
	 float4	n_Bt = tex2D	(s_dn_b, I.tcdbump).wzyx;
	 float4	n_At = tex2D	(s_dn_a, I.tcdbump).wzyx;
	 
 	 float3	n_R = (n_Rt-0.5)*mask.r; float g_R=n_Rt.w*mask.r;
	 float3	n_G = (n_Gt-0.5)*mask.g; float g_G=n_Gt.w*mask.g;
	 float3	n_B = (n_Bt-0.5)*mask.b; float g_B=n_Bt.w*mask.b;
	 float3	n_A = (n_At-0.5)*mask.a; float g_A=n_At.w*mask.a;

	 float3	mix		= 	n_R+n_G+n_B+n_A;
			mix.z	*=	0.5;		//. make bump twice as contrast (fake, remove me if possible)
	 float3	N		= 	mul     	(float3x3(I.M1, I.M2, I.M3), mix.xyz);

	 	 	G 		= 	g_R+g_G+g_B+g_A;
//			G 	= 0			;
//			G 	= mix.w		;		// gloss
  # else
	float3	N 	= I.N.xyz	;
  # endif

 # else
	D.rgb	= 2*D.rgb*tex2D	(s_detail, I.tcdbump).rgb;
 # endif
# else
	float3	N 	= I.N.xyz	;
# endif

	// hemi, sun, material
	float 	ms	= xmaterial	;
# ifdef USE_R2_STATIC_SUN
		 	ms 	= L.w		;
# endif

  // 2. Standart output
  O.Ne          = float4		(normalize(N), 									D.w			);	// hemi
  O.position    = float4 	(I.position.xyz + O.Ne.xyz*def_virtualh/2.h, 	ms			);	//
  O.C			= float4		(D.x,	D.y,	D.z,							G			);	// OUT: rgb.gloss

  return O;
}
