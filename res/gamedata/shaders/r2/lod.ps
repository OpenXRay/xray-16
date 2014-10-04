#include "common.h"
#include "sload.h"

struct 	vf
{
	float4 	hpos	: POSITION	;
	half3	position: TEXCOORD0	;
 	float2 	tc0	: TEXCOORD1	;	// base0
 	float2 	tc1	: TEXCOORD2	;	// base1
	half4 	af	: COLOR1	;	// alpha&factor
};

f_deffer 	main	( vf I )
{
  f_deffer	O;

  // 1. Base texture + kill pixels with low alpha
  half4 D0 	= tex2D		(s_base, I.tc0);
  half4 D1 	= tex2D		(s_base, I.tc1);
  half4 H0 	= tex2D		(s_hemi, I.tc0);	H0.xyz 	=  H0.rgb*2-1;	// expand
  half4 H1 	= tex2D		(s_hemi, I.tc1);	H1.xyz 	=  H1.rgb*2-1;	// expand

  half4 D 	= lerp		(D0,D1,I.af.w);		D.w	*= I.af.z;	// multiply alpha
  half4 H 	= lerp		(H0,H1,I.af.w);		H.w	*= I.af.x;	// multiply hemi

  clip		(D.w-(96.h/255.h))  ;
  half3 N 	= normalize	(H.xyz);

  // 2. Standart output
  O.Ne          = half4		(N					, H.w		);
  O.position    = half4 	(I.position + N*def_virtualh/2.h	, 0		);
  O.C		= half4		(D.x,	D.y,	D.z			, def_gloss	);		// OUT: rgb.gloss
  return O;
}
