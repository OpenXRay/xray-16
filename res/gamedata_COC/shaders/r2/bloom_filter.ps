#include "common.h"

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

//////////////////////////////////////////////////////////////////////////////////////////
uniform half4 		weight[2];

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
// Separable gauss filter: 	2*7 + 1 + 7*2 = 29 samples
// Samples:			0-central, -1, -2,..., -7, 1, 2,... 7
// Approximated i-count:	15t + 15a + 7a(d) + 1(out) = 38, HLSL compiled to 38 :)
half4 	main		( v2p I )	: COLOR
{
	// central
	half4 accum 	=	weight[1].w * tex2D	(s_bloom, I.tc0);

	// left (7)
	// right (7) - no swizles on 'texld', so this is dep-read infact
	accum		+= 	weight[0].x * tex2D	(s_bloom, I.tc1.xy);
	accum		+= 	weight[0].x * tex2D	(s_bloom, I.tc1.wz);

	accum		+= 	weight[0].y * tex2D	(s_bloom, I.tc2.xy);
	accum		+= 	weight[0].y * tex2D	(s_bloom, I.tc2.wz);

	accum		+= 	weight[0].z * tex2D	(s_bloom, I.tc3.xy);
	accum		+= 	weight[0].z * tex2D	(s_bloom, I.tc3.wz);

	accum		+= 	weight[0].w * tex2D	(s_bloom, I.tc4.xy);
	accum		+= 	weight[0].w * tex2D	(s_bloom, I.tc4.wz);

	accum		+= 	weight[1].x * tex2D	(s_bloom, I.tc5.xy);
	accum		+= 	weight[1].x * tex2D	(s_bloom, I.tc5.wz);

	accum		+= 	weight[1].y * tex2D	(s_bloom, I.tc6.xy);
	accum		+= 	weight[1].y * tex2D	(s_bloom, I.tc6.wz);

	accum		+= 	weight[1].z * tex2D	(s_bloom, I.tc7.xy);
	accum		+= 	weight[1].z * tex2D	(s_bloom, I.tc7.wz);

	// OK
	return 		accum;
}
