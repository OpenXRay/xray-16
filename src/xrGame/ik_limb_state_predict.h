#pragma once

struct ik_limb_state_predict
{
	float					time_to_footstep;
	float					footstep_shift;
	ik_foot_collider		collider;
	ik_limb_state_predict() : time_to_footstep( FLT_MAX ), footstep_shift(0){}
};