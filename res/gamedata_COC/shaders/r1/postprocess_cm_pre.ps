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
uniform sampler2D 	s_distort;
uniform sampler2D 	s_grad0;
uniform sampler2D 	s_grad1;
uniform half4 		c_colormap;

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_4	( v2p I )	: COLOR
{
	half4	pre_pass= tex2D		(s_base0, I.tc0);
	half	grad_i 	= dot		(pre_pass.rgb,(0.3333h).xxx);

	half3 	image0 	= tex2D		(s_grad0, half2(grad_i,0.5));
	half3 	image1 	= tex2D		(s_grad1, half2(grad_i,0.5));
	half3	image	= lerp		(image0, image1, c_colormap.y);
			image	= lerp		(pre_pass.rgb, image, c_colormap.x);

	return  half4	(image,1);					// +mov
}
