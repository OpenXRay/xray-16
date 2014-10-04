#include "common.h"
#include "lmodel.h"
#include "shadow.h"

struct 	_input
{
	float2 	tc 	: TEXCOORD0;
	float4 	tcJ : TEXCOORD1;
	float2	LT	: TEXCOORD2;
	float2	RT	: TEXCOORD3;
	float2	LB	: TEXCOORD4;
	float2 	RB	: TEXCOORD5;
};

float4 	main	( _input I ) : COLOR
{
  float4 _P		= tex2D 	(s_position, 	I.tc); 
  half4  _N		= tex2D 	(s_normal,   	I.tc); 

	// ----- light-model
	half 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif
	half4	light	= plight_infinity (m,_P,_N,Ldynamic_dir);

	// ----- shadow
	half4 	s_sum;
		s_sum.x	= tex2D	(s_smap,I.LT).x;
		s_sum.y = tex2D	(s_smap,I.RT).y;
		s_sum.z	= tex2D	(s_smap,I.LB).z;
		s_sum.w = tex2D	(s_smap,I.RB).w;
	half 	s 	= dot	(s_sum, 1.h/4.h);

	return 		Ldynamic_color * light * s;
}
