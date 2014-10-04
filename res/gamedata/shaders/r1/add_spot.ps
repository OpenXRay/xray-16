#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( vf_spot I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,I.tc0);
	half4	t_lmap 	= tex2D		(s_lmap,I.tc1);	// spot-projector
	half4 	t_att 	= tex2D		(s_att, I.tc2);	// spot-att

	half4 	final_color 	= t_base*t_lmap*t_att*I.color;
		final_color.rgb *= t_base.a;
	half3	final_rgb 	= (final_color+final_color)*2;
	half 	final_a 	= final_color.w;
	
	// out
	return  final_color*2;	//half4	(final_rgb,final_a);
}
