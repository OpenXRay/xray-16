#include "common.h"

struct 	v2p
{
 	float2 	tc0	: 	TEXCOORD0;	// base
	half4	c	:	COLOR0;		// diffuse

#ifdef	USE_SOFT_PARTICLES
//	Igor: for additional depth dest
	float4 tctexgen	: TEXCOORD1;
#endif	//	USE_SOFT_PARTICLES
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
uniform sampler s_distort;
half4 	main	( v2p I )	: COLOR
{
	half4	distort	= tex2D		(s_distort,I.tc0);
	half    factor	= distort.a * dot(I.c.rgb,0.33h);
/*
#ifdef	USE_SOFT_PARTICLES
	half2	zero = half2( 0.5, 0.5);
	half	alphaDistort;
	half4 _P = tex2Dproj( s_position, I.tctexgen);
	half spaceDepth = _P.z - I.tctexgen.z;
	if (spaceDepth < -0.1h ) spaceDepth = 100000.0h; //  Skybox doesn't draw into position buffer
	alphaDistort = saturate(1.3*spaceDepth);
//	alphaDistort = 0;
	distort.xy = lerp  ( zero, distort.xy, alphaDistort);
#endif	//	USE_SOFT_PARTICLES
*/
	return	half4	(distort.rgb,factor);
}
