#include "common.h"

struct 	v2p
{
 	half2 	tc0: 		TEXCOORD0;	// lmap
 	half2 	tc1: 		TEXCOORD1;	// lmap
	half3	c0:		COLOR0;		// hemi
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_lmap 	= tex2D		(s_lmap,I.tc1);

	// lighting
	half3 	l_base 	= t_lmap.rgb;			// base light-map
	half3	l_hemi 	= I.c0 * t_base.a;		// hemi is implicitly inside texture
	half 	l_sun 	= t_lmap.a;			// sun color
	half3	light	= L_ambient + l_base + l_hemi	;
	return  half4	(light,l_sun);
}
