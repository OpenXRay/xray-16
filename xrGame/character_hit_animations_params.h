#pragma once

struct hit_animation_global_params
{
	hit_animation_global_params	( )		;
	float	power_factor				;// 2.f;
	float	rotational_power_factor		;
	float	side_sensitivity_threshold	;
	float	anim_channel_factor			;

	float	block_blend					;
	float	reduce_blend				;
	float	reduce_power_factor			;

} ;
extern	hit_animation_global_params		ghit_anims_params	;
extern	int			tune_hit_anims							;