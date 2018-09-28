#include "common.h"
#include "lmodel.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
// Note: this is a half-sphere
uniform half3		direction;
half4 	main		( float4 tc:TEXCOORD0 )	: COLOR
{
  float4 _P		= tex2Dproj 	(s_position, 	tc); 
  half4 _N		= tex2Dproj 	(s_normal,   	tc); 

  half3 	L2P 	= _P.xyz - Ldynamic_pos.xyz;                         		// light2point
  half3 	L2P_N 	= normalize	(L2P); 	                        		// light2point
  half 		rsqr	= dot		(L2P,L2P);					// distance 2 light (squared)
  half  	att 	= saturate	(1 - rsqr*Ldynamic_pos.w);			// q-linear attenuate
  half  	light	= saturate	(dot(-L2P_N,_N.xyz));
  half 		hemi 	= saturate	(dot(L2P_N, direction));

  // Final color
	return 	blendp	(half4(Ldynamic_color.xyz * att * light * hemi, 0), tc);
}
