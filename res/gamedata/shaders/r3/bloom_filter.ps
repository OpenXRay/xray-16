#include "common.h"
/*
struct 	v2p
{
  float2 tc0: 		TEXCOORD0;	// Central
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
uniform float4 		weight[2];

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
// Separable gauss filter: 	2*7 + 1 + 7*2 = 29 samples
// Samples:			0-central, -1, -2,..., -7, 1, 2,... 7
// Approximated i-count:	15t + 15a + 7a(d) + 1(out) = 38, HLSL compiled to 38 :)
float4 main ( p_filter I ) : SV_Target
{
	// central
	//float4 accum 	=	weight[1].w * tex2D	(s_bloom, I.tc0);
	float4 accum 	=	weight[1].w * s_bloom.Sample(smp_rtlinear, I.Tex0);

	// left (7)
	// right (7) - no swizles on 'texld', so this is dep-read infact
//	accum		+= 	weight[0].x * tex2D	(s_bloom, I.tc1.xy);
//	accum		+= 	weight[0].x * tex2D	(s_bloom, I.tc1.wz);
	accum		+= 	weight[0].x * s_bloom.Sample(smp_rtlinear, I.Tex1.xy);
	accum		+= 	weight[0].x * s_bloom.Sample(smp_rtlinear, I.Tex1.wz);

//	accum		+= 	weight[0].y * tex2D	(s_bloom, I.tc2.xy);
//	accum		+= 	weight[0].y * tex2D	(s_bloom, I.tc2.wz);
	accum		+= 	weight[0].y * s_bloom.Sample(smp_rtlinear, I.Tex2.xy);
	accum		+= 	weight[0].y * s_bloom.Sample(smp_rtlinear, I.Tex2.wz);

//	accum		+= 	weight[0].z * tex2D	(s_bloom, I.tc3.xy);
//	accum		+= 	weight[0].z * tex2D	(s_bloom, I.tc3.wz);
	accum		+= 	weight[0].z * s_bloom.Sample(smp_rtlinear, I.Tex3.xy);
	accum		+= 	weight[0].z * s_bloom.Sample(smp_rtlinear, I.Tex3.wz);

//	accum		+= 	weight[0].w * tex2D	(s_bloom, I.tc4.xy);
//	accum		+= 	weight[0].w * tex2D	(s_bloom, I.tc4.wz);
	accum		+= 	weight[0].w * s_bloom.Sample(smp_rtlinear, I.Tex4.xy);
	accum		+= 	weight[0].w * s_bloom.Sample(smp_rtlinear, I.Tex4.wz);

//	accum		+= 	weight[1].x * tex2D	(s_bloom, I.tc5.xy);
//	accum		+= 	weight[1].x * tex2D	(s_bloom, I.tc5.wz);
	accum		+= 	weight[1].x * s_bloom.Sample(smp_rtlinear, I.Tex5.xy);
	accum		+= 	weight[1].x * s_bloom.Sample(smp_rtlinear, I.Tex5.wz);

//	accum		+= 	weight[1].y * tex2D	(s_bloom, I.tc6.xy);
//	accum		+= 	weight[1].y * tex2D	(s_bloom, I.tc6.wz);
	accum		+= 	weight[1].y * s_bloom.Sample(smp_rtlinear, I.Tex6.xy);
	accum		+= 	weight[1].y * s_bloom.Sample(smp_rtlinear, I.Tex6.wz);

//	accum		+= 	weight[1].z * tex2D	(s_bloom, I.tc7.xy);
//	accum		+= 	weight[1].z * tex2D	(s_bloom, I.tc7.wz);
	accum		+= 	weight[1].z * s_bloom.Sample(smp_rtlinear, I.Tex7.xy);
	accum		+= 	weight[1].z * s_bloom.Sample(smp_rtlinear, I.Tex7.wz);

	// OK
	return 		accum;
}
