#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main_ps_1_1	( vf_point I )	: COLOR
{
	half4	t_base 	= tex2D		(s_base,	I.tc0);
	half4 	t_att1 	= tex2D		(s_lmap, 	I.tc1);	// point-att
	half4 	t_att2 	= tex2D		(s_att, 	I.tc2);	// point-att
	half4 	t_att 	= t_att1*t_att2;

	half4 	final_color 	= t_base*t_att*I.color;
		final_color.rgb *= t_base.a;
	half3	final_rgb 	= (final_color+final_color)*2;
	half 	final_a 	= final_color.w;
	
	// out
	return  final_color*2;	//half4	(final_rgb,final_a);
}
