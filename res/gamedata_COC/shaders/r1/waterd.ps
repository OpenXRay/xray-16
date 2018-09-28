#include "common.h"
#include "shared\waterconfig.h"

struct   v2p
{
	half2	tbase:		TEXCOORD0;  // base
	half2	tdist0:		TEXCOORD1;  // d0
	half2	tdist1:		TEXCOORD2;  // d1
};

uniform sampler2D	s_distort0;
uniform sampler2D	s_distort1;
//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base		= tex2D	(s_base,  	I.tbase  );

	half2	t_d0		= tex2D (s_distort0,   	I.tdist0 );
	half2	t_d1		= tex2D (s_distort1,   	I.tdist1 );
	half2	distort		= (t_d0+t_d1)*0.5;      		// average
	half2	zero		= half2	(0.5,0.5);
	half2	faded		= lerp  (distort,zero,	t_base.a );

	half2	faded_bx2	= (faded*2-1)*W_DISTORT_POWER;
	half 	faded_dot	= dot	(half3(faded_bx2,0),.75	);	//0.75
	half 	alpha		= 0.5;		//abs  	(faded_dot);
  // out
return  half4  (faded,0,alpha);
}
