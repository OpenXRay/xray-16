#ifndef skeleton_motion_defs_inluded
#define skeleton_motion_defs_inluded

#pragma once

const	u32		MAX_PARTS			=	4;

const	f32		SAMPLE_FPS			=	30.f;
const	f32		SAMPLE_SPF			=	(1.f/SAMPLE_FPS);
f32		const	END_EPS				=	SAMPLE_SPF+EPS;
const	f32		KEY_Quant			=	32767.f;
const	f32		KEY_QuantI			=	1.f/KEY_Quant;

#endif
