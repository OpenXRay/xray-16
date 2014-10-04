#include "common.h"

struct 	v2p
{
 	half2 	tc0: 		TEXCOORD0;	// base
 	half2 	tc1: 		TEXCOORD1;	// lmap
 	half2 	tc2: 		TEXCOORD2;	// hemi
 	half2 	tc3: 		TEXCOORD3;	// detail
  	half4	c0:		COLOR0;		// c0.a * 
	half4	c1:	        COLOR1;		// c1.a + 
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
	half3	light	= L_ambient + l_base + l_sun + l_hemi;

	// calc D-texture
	half4	t_dt 	= tex2D		(s_detail,I.tc3);
	half3 	detail	= t_dt*I.c0.a + I.c1.a;
	
	// final-color
	half3	final 	= (light*t_base*2)*detail*2;
	
	// out
	return  half4	(final.r,final.g,final.b,t_base.a);
}
