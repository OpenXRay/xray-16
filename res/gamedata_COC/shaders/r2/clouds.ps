#include "common.h"

struct 	v2p
{
	half4	color	: COLOR0;	// rgb. intensity, for SM3 - tonemap prescaled
  	half2	tc0		: TEXCOORD0;
  	half2	tc1		: TEXCOORD1;
};

uniform sampler2D 	s_clouds0	: register(s0);
uniform sampler2D 	s_clouds1	: register(s1);

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main	( v2p I )	: COLOR
{
	half4 	s0		= tex2D		(s_clouds0,I.tc0);
	half4 	s1		= tex2D		(s_clouds1,I.tc1);
	half4 	mix 	= I.color * (s0 + s1)	;

#ifdef	USE_VTF
	half4 	rgb		= mix		;
#else
	half 	scale 	= tex2D		(s_tonemap,half2(.5h,.5h)).x;
	half4 	rgb , hi;
	tonemap	(rgb, hi, mix, scale );
#endif

	return  half4	(rgb.rgb, rgb.a);
}
