#include "common.h"

struct 	v2p
{
 	half2 	tc0: 		TEXCOORD0;	// base
 	half2 	tc1: 		TEXCOORD1;	// lmap
  	half3	c0:		COLOR0;		// sun
  	half4	c1:		COLOR1;		// lq-color + factor
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D	(s_base,I.tc0);
	half4	t_lmap 	= tex2D	(s_lmap,I.tc1);

	// lighting
	half3 	l_base 	= t_lmap.rgb;				// base light-map (lmap color, ambient, hemi, etc - inside)
	half3 	l_sun 	= I.c0*t_lmap.a;			// sun color
	half3	light	= lerp	(l_base + l_sun, I.c1, I.c1.w);

	// final-color
	half3	final 	= light*t_base*2;

	// out
	return  half4	(final.r,final.g,final.b,t_base.a);
}
