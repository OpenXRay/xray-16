#include "common.h"

struct 	v2p
{
	half4 	factor:	COLOR0;
  	half3	tc0:	TEXCOORD0;
  	half3	tc1:	TEXCOORD1;
};

uniform sampler2D 	s_sun0;
uniform sampler2D 	s_sun1;
uniform half4 		color;		// lerped color + alpha for alpha blend

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half3 	s0	= tex2D(s_sun0,I.tc0);
	half3 	s1	= tex2D(s_sun1,I.tc1);
	

	half3	sun	= color*lerp(s0,s1,I.factor.w);

	return  half4	(sun,color.w);
}
