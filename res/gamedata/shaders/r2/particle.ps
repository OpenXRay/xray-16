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


//	Must be less than view near
#define	DEPTH_EPSILON	0.1h
//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main	( v2p I )	: COLOR
{
	half4 result = I.c*tex2D	(s_base,I.tc0);

	//	Igor: additional depth test
#ifdef	USE_SOFT_PARTICLES
	half4 _P               = tex2Dproj         (s_position,         I.tctexgen);
	half spaceDepth = _P.z-I.tctexgen.z-DEPTH_EPSILON;
	if (spaceDepth < -2*DEPTH_EPSILON ) spaceDepth = 100000.0h; //  Skybox doesn't draw into position buffer
	//result.a *= saturate(spaceDepth*0.3h);
	result.a *= Contrast( saturate(spaceDepth*1.3h), 2);
	result.rgb *= Contrast( saturate(spaceDepth*1.3h), 2);
//	result = Contrast( saturate(spaceDepth*1.3h), 2);
//	result = saturate (spaceDepth*5.0);
//	result.a = 1;
#endif	//	USE_SOFT_PARTICLES

	return	result;
}
