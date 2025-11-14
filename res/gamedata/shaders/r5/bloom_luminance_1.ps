#include "common.h"
/*
struct 	v2p
{
  float4 tc0: 		TEXCOORD0;	// Central
  float4 tc1: 		TEXCOORD1;	// -1,+1
  float4 tc2: 		TEXCOORD2;	// -2,+2
  float4 tc3: 		TEXCOORD3;	// -3,+3
  float4 tc4: 		TEXCOORD4;	// -4,+4
  float4 tc5: 		TEXCOORD5;	// -5,+5
  float4 tc6: 		TEXCOORD6;	// -6,+6
  float4 tc7: 		TEXCOORD7;	// -7,+7
};
*/

//////////////////////////////////////////////////////////////////////////////////////////
#define	LUMINANCE_BASE		0.0001h

float	luminance	(float2	tc)	
{
	//float3	source 	= tex2D(s_image,tc);
	float3	source 	= s_image.Sample( smp_rtlinear, tc );
	return 	dot( source, LUMINANCE_VECTOR*def_hdr );
}
//////////////////////////////////////////////////////////////////////////////////////////
// 	perform 2x2=4s convolution, working on 4x4=16p area
//	that means 256x256 source will be scaled to (256/4)x(256/4) = 64x64p
//	a):	256x256 => 64x64p	with log 
//	b):	64x64p	=> 8x8p
//	c):	8x8p	=> 1x1p		with exp
float4 main( p_build I ) : SV_Target
{
	// first 8 bilinear samples (8x4 = 32 pixels)
	float4 	final;
		final.x =	luminance(I.Tex0);
		final.y = 	luminance(I.Tex1);
		final.z = 	luminance(I.Tex2);
		final.w =	luminance(I.Tex3);

	// OK
	return 		final	;
}
