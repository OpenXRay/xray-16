#include "common.h"

struct 	v2p
{
	float4	color	: COLOR0;	// rgb. intensity, for SM3 - tonemap prescaled
  	float2	tc0		: TEXCOORD0;
  	float2	tc1		: TEXCOORD1;
};

//uniform sampler2D 	s_clouds0	: register(s0);
//uniform sampler2D 	s_clouds1	: register(s1);
Texture2D	s_clouds0	: register(t0);
Texture2D	s_clouds1	: register(t1);

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4 	main	( v2p I )	: SV_Target
{
//	float4 	s0		= tex2D		(s_clouds0,I.tc0);
//	float4 	s1		= tex2D		(s_clouds1,I.tc1);
	float4 	s0		= s_clouds0.Sample( smp_base, I.tc0 );
	float4 	s1		= s_clouds1.Sample( smp_base, I.tc1 );
	float4 	mix 	= I.color * (s0 + s1)	;

	float4 	rgb		= mix		;

//	return  float4	(rgb.rgb, I.color.a);
	return  rgb;
}
