#include "common.h"

struct 	v2p
{
 	half2 	tc0: 		TEXCOORD0;	// base
	half3	tc1:		TEXCOORD1;	// environment
 	half2 	tc2: 		TEXCOORD2;	// lmap
  	half3	c0:		COLOR0;		// sun
  	half4	c1:		COLOR1;		// lq-color + factor
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_env 	= texCUBE	(s_env,	I.tc1);

	// final-color
	half3 	final 	= lerp	(t_env,t_base,t_base.a);

	// out
	return  half4	(final.r,final.g,final.b,t_base.a);
}
