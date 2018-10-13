#include "common.h"
#include "mblur.h"

struct 	v2p
{
  float4 tc0: 		TEXCOORD0;	// Center
  float4 tc1: 		TEXCOORD1;	// LT 		 
  float4 tc2: 		TEXCOORD2;	// RB
  float4 tc3: 		TEXCOORD3;	// RT 
  float4 tc4: 		TEXCOORD4;	// LB
  float4 tc5:		TEXCOORD5;	// Left	/ Right	
  float4 tc6:		TEXCOORD6;	// Top  / Bottom 
};

//////////////////////////////////////////////////////////////////////////////////////////
uniform sampler2D 	s_distort;
uniform half4 		e_barrier;	// x=norm(.8f), y=depth(.1f), z=clr
uniform half4 		e_weights;	// x=norm, y=depth, z=clr
uniform half4 		e_kernel;	// x=norm, y=depth, z=clr

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main		( v2p I )	: COLOR
{
  // Normal discontinuety filter
  half3 nc		=  tex2D	(s_normal,   	I.tc0);
  half4 nd;
	nd.x 		=  dot 		(nc, (half3)tex2D(s_normal,I.tc1));
	nd.y 		=  dot 		(nc, (half3)tex2D(s_normal,I.tc2));
	nd.z 		=  dot 		(nc, (half3)tex2D(s_normal,I.tc3));
	nd.w 		=  dot 		(nc, (half3)tex2D(s_normal,I.tc4));
	nd 		-= e_barrier.x	;
	nd 		=  step		(0,nd);	// bw
  half  ne 		=  saturate	(dot(nd,e_weights.x));

  // Opposite coords
  float4 tc5r 		=  I.tc5.wzyx;
  float4 tc6r 		=  I.tc6.wzyx;

  // Depth filter : compute gradiental difference: (c-sample1)+(c-sample1_opposite)
  half4 dc		=  tex2D 	(s_position, I.tc0); 
  half4 dd;
	dd.x 		=  (half)tex2D(s_position,I.tc1).z + (half)tex2D(s_position,I.tc2).z;
	dd.y 		=  (half)tex2D(s_position,I.tc3).z + (half)tex2D(s_position,I.tc4).z;
	dd.z 		=  (half)tex2D(s_position,I.tc5).z + (half)tex2D(s_position,tc5r).z;
	dd.w 		=  (half)tex2D(s_position,I.tc6).z + (half)tex2D(s_position,tc6r).z;
	dd 		=  abs(2*dc.z-dd)-e_barrier.y;
	dd 		=  step		(dd,0);		// bw
  half  de 		=  saturate	(dot(dd,e_weights.y));

  // weight
  half 	w 		=  (1-de*ne)*e_kernel.x;	// 0 - no aa, 1=full aa

#ifdef 	USE_DISTORT
	half4 	distort	= tex2D		(s_distort, I.tc0);
	half2	doffs	= (distort.xy-.5h)*def_distort;
	float2	center	= I.tc0 + doffs;
#else
	float2	center 	= I.tc0;
#endif

  // Smoothed color
  // (a-c)*w + c = a*w + c(1-w)

	float2	offset 	=  center * (1-w);
  half4 	s0 	=  tex2D	(s_image, offset + I.tc1*w);
  half4 	s1 	=  tex2D	(s_image, offset + I.tc2*w);
  half4 	s2 	=  tex2D	(s_image, offset + I.tc3*w);
  half4 	s3 	=  tex2D	(s_image, offset + I.tc4*w);

	half3 	final	=  mblur	(center, dc, (s0+s1+s2+s3)/4.h);

  return combine_bloom(final,tex2D	(s_bloom, I.tc0));
}
