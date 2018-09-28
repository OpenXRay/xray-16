#include "common.h"

struct 	v2p
{
 	half2 	tc0: 		TEXCOORD0;	// base
 	half2 	tc1: 		TEXCOORD1;	// lmap
 	half2 	tc2: 		TEXCOORD2;	// lmap-hemi
  	half3	c0:		COLOR0;		// hemi
	half3	c1:	        COLOR1;		// sun
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_lmap 	= tex2D		(s_lmap,I.tc1);

	// lighting
	half3 	l_base 	= t_lmap.rgb;				// base light-map
	half3	l_hemi 	= I.c0*p_hemi(I.tc2);			// hemi
	half3 	l_sun 	= I.c1*t_lmap.a;			// sun color
	half3	light	= L_ambient + l_base + l_sun + l_hemi	;

	// final-color
	half3	final 	= light*t_base*2;
	
	// out
	return  half4	(final,t_base.a);
}
