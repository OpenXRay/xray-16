#include "common.h"
#include "sload.h"

struct 	p_particle
{
	p_flat	base	;
	half4 	color	: COLOR0;
};

f_deffer 	main	( p_particle II )
{
  f_deffer	O;
  p_flat	I;	I=II.base;

  // 1. Base texture + kill pixels with low alpha
  half4 	D 	= 	tex2D		(s_base, I.tcdh);
			D	*=	II.color;
			clip	(D.w-def_aref);

  // 2. Standart output
  O.Ne          = half4		(normalize((half3)I.N.xyz)					, I.position.w	);
  O.position    = half4 	(I.position.xyz + O.Ne.xyz*def_virtualh/2.h	, xmaterial		);
  O.C			= half4		(D.xyz,			def_gloss);		// OUT: rgb.gloss
  return O;
}
