#include "common.h"

struct 	v2p
{
	half4	color	: COLOR0;	// rgb. intensity
  	half2	tc0	: TEXCOORD0;
  	half2	tc1	: TEXCOORD1;
};

uniform sampler2D 	s_clouds0	: register(s0);
uniform sampler2D 	s_clouds1	: register(s1);

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4 	s0	= tex2D		(s_clouds0,I.tc0);
	half4 	s1	= tex2D		(s_clouds1,I.tc1);
	half4 	mix 	= I.color * (s0 + s1)	;

	return  half4	(mix.rgb, mix.a)	;
}
