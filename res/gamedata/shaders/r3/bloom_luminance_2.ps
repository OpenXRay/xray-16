#include "common.h"

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

//////////////////////////////////////////////////////////////////////////////////////////
//	perform 4x4 bilinear, 8x8p, the step (B)
//	b):	64x64p	=> 8x8p
float	sample	(float2	tc)	
{
//	float4	data = tex2D(s_image,tc);
	float4	data = s_image.Sample( smp_rtlinear, tc );
	return 	dot( data, 1.h/4.h );	// sum components
}

float4 main( p_filter I ) : SV_Target
{
	// sample
	float4 	accum0;
		accum0.x =	sample(I.Tex0);
		accum0.y = 	sample(I.Tex1);
		accum0.z = 	sample(I.Tex2);
		accum0.w =	sample(I.Tex3);
	float4 	accum1;
		accum1.x =	sample(I.Tex4);
		accum1.y = 	sample(I.Tex5);
		accum1.z = 	sample(I.Tex6);
		accum1.w =	sample(I.Tex7);
	float4 	accum2;
		accum2.x =	sample(I.Tex0.wz);
		accum2.y = 	sample(I.Tex1.wz);
		accum2.z = 	sample(I.Tex2.wz);
		accum2.w =	sample(I.Tex3.wz);
	float4 	accum3;
		accum3.x =	sample(I.Tex4.wz);
		accum3.y = 	sample(I.Tex5.wz);
		accum3.z = 	sample(I.Tex6.wz);
		accum3.w =	sample(I.Tex7.wz);

	// perform accumulation
	float4	final;
		final.x	= dot(accum0,1.h/4.h);
		final.y	= dot(accum1,1.h/4.h);
		final.z	= dot(accum2,1.h/4.h);
		final.w	= dot(accum3,1.h/4.h);

	// OK
	return 	final;
}
