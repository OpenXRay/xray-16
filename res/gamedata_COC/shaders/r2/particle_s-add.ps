#include "common.h"

struct 	v2p
{
 	float2 	tc0	: 	TEXCOORD0;	// base
	half4	c	:	COLOR0;		// diffuse
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main	( v2p I )	: COLOR
{
	// color = 0 	-> color=1
	// color = 1	-> color=c
	half4	c 	= I.c*tex2D	(s_base,I.tc0);
	half3 	r	= half3(1,1,1)-c.xyz-c.xyz*c.xyz;	// lerp(1,c.xyz,c.xyz), can't be less than .5h
	return	half4	(r,1);
}
