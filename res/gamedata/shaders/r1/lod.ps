#include "common.h"

struct 	v2p
{
 	float2 	tc0: 	TEXCOORD0;	// base0
 	float2 	tc1: 	TEXCOORD1;	// base1
 	float2 	tc2: 	TEXCOORD2;	// hemi0
 	float2 	tc3: 	TEXCOORD3;	// hemi1
	half4 	c:	COLOR0;		// color
	half4 	f:	COLOR1;		// color
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
uniform sampler2D 	s_base0	;
uniform sampler2D 	s_base1	;
uniform sampler2D 	s_hemi0	;
uniform sampler2D 	s_hemi1	;
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4 	base0 		= 	tex2D	(s_base0,I.tc0);
	half4 	base1 		= 	tex2D	(s_base1,I.tc1);
	half4 	hemi0 		= 	tex2D	(s_hemi0,I.tc2);
	half4 	hemi1 		= 	tex2D	(s_hemi1,I.tc3);

	half4 	base 		= 	lerp	(base0,base1,I.f.w)*I.c	;
	half 	hemi 		= 	lerp	(hemi0,hemi1,I.f.w).w	;

	half3 	color 		= 	base*2* (0.5+0.5*hemi);
	return 	half4 		(color,base.a);
}
