#include "common.h"

struct 	v2p
{
 	float2 	tc0: 		TEXCOORD0;	// base
 	half3 	tc1: 		TEXCOORD1;	// environment
  	half4	c0:			COLOR0;		// sun.(fog*fog)
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
uniform samplerCUBE 	s_env	;
half4 	main	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_env 	= texCUBE	(s_env,	I.tc1);

	half3 	base 	= lerp		(t_env,t_base,t_base.a);
	half3	light	= I.c0;
	half3	final 	= light*base*2;

	// out
	return  half4	(final.r,final.g,final.b,t_base.a*I.c0.a);
}
