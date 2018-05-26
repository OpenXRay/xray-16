#include "common.h"
#include "sload.h"

f_deffer 	main	( p_bumped I )
{
  f_deffer	O;

  surface_bumped 	S 	= sload 	(I);

  // Sample normal, rotate it by matrix, encode position 
  float3 Ne  = mul       (float3x3(I.M1, I.M2, I.M3), S.normal);
		Ne	= normalize	(Ne);
  O.Ne 		= float4		(Ne, S.base.w);
  O.position= float4 	(I.position.xyz + Ne*S.height*def_virtualh, xmaterial);
  O.C		= float4		(S.base.x,S.base.y,S.base.z,S.gloss);		// OUT: rgb.gloss

  return O;
}
