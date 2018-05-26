#include "common.h"
#include "lmodel.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
// Note: this is a float-sphere
uniform float3		direction;
float4 	main		( float4 tc:TEXCOORD0 )	: COLOR
{
  float4 _P		= tex2Dproj 	(s_position, 	tc); 
  float4 _N		= tex2Dproj 	(s_normal,   	tc); 

  float3 	L2P 	= _P.xyz - Ldynamic_pos.xyz;                         		// light2point
  float3 	L2P_N 	= normalize	(L2P); 	                        		// light2point
  float 		rsqr	= dot		(L2P,L2P);					// distance 2 light (squared)
  float  	att 	= saturate	(1 - rsqr*Ldynamic_pos.w);			// q-linear attenuate
  float  	light	= saturate	(dot(-L2P_N,_N.xyz));
  float 		hemi 	= saturate	(dot(L2P_N, direction));

  // Final color
	return 	blendp	(float4(Ldynamic_color.xyz * att * light * hemi, 0), tc);
}
