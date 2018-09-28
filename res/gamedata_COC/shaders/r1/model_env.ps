#include "common.h"

struct 	v2p
{
  	half4	diffuse:	COLOR0;
 	half4 	tc0: 		TEXCOORD0;	// projector
 	half4 	tc1: 		TEXCOORD1;	// env
	half4	tc2:		TEXCOORD2;	// base
};


//////////////////////////////////////////////////////////////////////////////////////////
uniform sampler2D 	s_projector;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	light 	= I.diffuse + tex2D(s_projector,I.tc0);
	half4	t_env 	= texCUBE	(s_env,	I.tc1);
	half4	t_base 	= tex2D		(s_base,I.tc2);
	half4 	base 	= lerp		(t_env,t_base,t_base.a);
	return  light*base*2;
}
