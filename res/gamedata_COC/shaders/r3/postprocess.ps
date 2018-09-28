#include "common.h"
/*
struct 	v2p
{
//D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX3
	float4 	color:		COLOR0;		// multiplier, color.w = noise_amount
	float4	gray:		COLOR1;		// (.3,.3,.3.,amount)
 	float2 	tc0: 		TEXCOORD0;	// base1 (duality)
 	float2 	tc1: 		TEXCOORD1;	// base2 (duality)
 	float2 	tc2: 		TEXCOORD2;	// base  (noise)
};
*/

Texture2D 	s_base0;
Texture2D 	s_base1;
Texture2D 	s_noise;
uniform float4 		c_brightness;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4 main( p_postpr I ) : SV_Target
{
//	float3	t_0 	= tex2D		(s_base0,I.tc0);
//	float3	t_1 	= tex2D		(s_base1,I.tc1);
	float3	t_0 	= s_base0.Sample( smp_rtlinear, I.Tex0);
	float3	t_1 	= s_base1.Sample( smp_rtlinear, I.Tex1);	
	float3 	image	= (t_0+t_1)*.5;					// add_d2
	float	gray 	= dot		(image,I.Gray);			// dp3
			image 	= lerp 		(gray,image,I.Gray.w);		// mul/mad

//	float4	t_noise	= tex2D		(s_noise,I.tc2);	
	float4	t_noise	= s_noise.Sample( smp_linear, I.Tex2);	
	float3 	noised 	= image*t_noise*2;                     		// mul_2x
			image	= lerp( noised, image, I.Color.w); 	// lrp ?
			image	= (image * I.Color + c_brightness)*2;		// mad
//		image	= (image + c_brightness) * I.Color;		// mad ?

	// out
	return  float4( image, 1.0h);					// +mov
}
