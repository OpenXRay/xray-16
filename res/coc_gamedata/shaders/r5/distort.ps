#include "common.h"

struct 	v2p
{
 	float2 	tc: 		TEXCOORD0;	// base & distort
};

// uniform sampler2D 	s_base;
//uniform sampler2D 	s_distort;
Texture2D 	s_distort;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4 main ( v2p I ) : SV_Target
{
//	float2 	distort	= tex2D		(s_distort, I.tc);
	float2 	distort	= s_distort.Sample( smp_rtlinear, I.tc);
	float2	offset	= (distort.xy-.5h)*def_distort;
	float3	image 	= s_base.Sample( smp_rtlinear, I.tc + offset);

	// out
	return  float4	(image,1);					// +mov
}
