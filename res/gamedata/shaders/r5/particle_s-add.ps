#include "common.h"

struct 	v2p
{
 	float2 	tc0	: 	TEXCOORD0;	// base
	float4	c	:	COLOR0;		// diffuse
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4 main ( v2p I ) : SV_Target
{
	// color = 0 	-> color=1
	// color = 1	-> color=c
//	float4	c 	= I.c*tex2D	(s_base,I.tc0);
	float4	c 	= I.c * s_base.Sample( smp_base, I.tc0 );
   float3 	r	= float3(1,1,1)-c.xyz-c.xyz*c.xyz;	// lerp(1,c.xyz,c.xyz), can't be less than .5h
	return	float4	(r,1);
}
