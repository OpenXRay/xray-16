#include "common.h"

struct 	v2p
{
 	half2 	tc0: 	TEXCOORD0;	// base
 	half2 	tc1: 	TEXCOORD1;	// base
  	half4	c0:	COLOR0;
  	half4	c1:	COLOR1;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( v2p I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_dt 	= tex2D		(s_detail,I.tc1);

	half3 	detail	= t_dt*I.c0.a + I.c1.a;
	half3	final 	= (t_base*I.c0*2)*detail*2;

	// out
	return  half4	(final.r,final.g,final.b,t_base.a);
}
