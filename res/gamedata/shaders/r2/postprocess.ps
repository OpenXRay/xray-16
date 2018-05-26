#include "common.h"

struct 	v2p
{
	half4 	color:		COLOR0;		// multiplier, color.w = noise_amount
	half4	gray:		COLOR1;		// (.3,.3,.3.,amount)
 	half2 	tc0: 		TEXCOORD0;	// base1 (duality)
 	half2 	tc1: 		TEXCOORD1;	// base2 (duality)
 	half2 	tc2: 		TEXCOORD2;	// base  (noise)
};

uniform sampler2D 	s_base0;
uniform sampler2D 	s_base1;
uniform sampler2D 	s_noise;
uniform half4 		c_brightness;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half3	t_0 	= tex2D		(s_base0,I.tc0);
	half3	t_1 	= tex2D		(s_base1,I.tc1);	
	half3 	image	= (t_0+t_1)*.5;					// add_d2
	half	gray 	= dot		(image,I.gray);			// dp3
		image 	= lerp 		(gray,image,I.gray.w);		// mul/mad

	half4	t_noise	= tex2D		(s_noise,I.tc2);	
	half3 	noised 	= image*t_noise*2;                     		// mul_2x
		image	= lerp 		(noised,image,I.color.w); 	// lrp ?
		image	= (image * I.color + c_brightness)*2;		// mad
//		image	= (image + c_brightness) * I.color;		// mad ?

	// out
	return  half4	(image,1);					// +mov
}
