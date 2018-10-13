#include "common.h"

struct 	v2p
{
	half4 	factor:	COLOR0;
  	half3	tc0:	TEXCOORD0;
  	half3	tc1:	TEXCOORD1;
};

uniform samplerCUBE 	s_sky0	: register(s0);
uniform samplerCUBE 	s_sky1	: register(s1);
uniform half3 		color;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half3 	s0	= texCUBE(s_sky0,I.tc0);
	half3 	s1	= texCUBE(s_sky1,I.tc1);
	half3	sky	= I.factor*lerp(s0,s1,I.factor.w);

	return  half4	(sky,1);
}
