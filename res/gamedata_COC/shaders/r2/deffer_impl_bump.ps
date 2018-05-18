#include "common.h"
#include "sload.h"

f_deffer 	main	( p_bumped I )
{
  f_deffer	O;

  surface_bumped 	S 	= sload 	(I);

  // Sample normal, rotate it by matrix, encode position 
  half3 Ne  = mul       (half3x3(I.M1, I.M2, I.M3), S.normal);
		Ne	= normalize	(Ne);
  O.Ne 		= half4		(Ne, S.base.w);
  O.position= half4 	(I.position.xyz + Ne*S.height*def_virtualh, xmaterial);
  O.C		= half4		(S.base.x,S.base.y,S.base.z,S.gloss);		// OUT: rgb.gloss

  return O;
}
