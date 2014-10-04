#include "common.h"

struct   v2p
{
   half2   	tbase:   	TEXCOORD0;  // base
   half3   	tenv:   	TEXCOORD1;  // env
   half4	c0:  		COLOR0;
};


////////////////////////////////////////////////////////////////////////////////
// Pixel
half4   main_ps_1_1  ( v2p I )  : COLOR
{

	half4	t_base		= tex2D   (s_base,I.tbase);
	half3	t_env		= texCUBE  (s_env, I.tenv);

	half3	refl		= lerp    (t_env,t_base,I.c0.a);
	half3	color		= lerp    (refl, t_base,t_base.a);
	half3	final		= color*I.c0*2  ;

	half	alpha_shift	= saturate(.5-I.c0.a);
	half	alpha_add	= alpha_shift*alpha_shift;
	half	alpha		= t_base.a;
	// out
  return  half4   (final,   t_base.a );  //t_base.a + (1-I.c0.a));  //half4  (final, t_base.a );
}
