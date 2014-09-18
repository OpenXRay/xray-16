#pragma once

#include "associative_vector.h"

#define MIN_LEGS_COUNT	1
#define MAX_LEGS_COUNT	4 

#include "../include/xrRender/animation_motion.h"

struct SStepParam {
	struct{
		float	time;
		float	power;
	} step[MAX_LEGS_COUNT];

	u8			cycles;
};

//DEFINE_MAP(MotionID, SStepParam, STEPS_MAP, STEPS_MAP_IT);
typedef associative_vector<MotionID, SStepParam>	STEPS_MAP;
typedef STEPS_MAP::iterator							STEPS_MAP_IT;

struct SStepInfo {
	struct {
		bool			handled;		// обработан
		u8				cycle;			// цикл в котором отработан
	} activity[MAX_LEGS_COUNT];

	SStepParam		params;
	bool			disable;

	u8				cur_cycle;

	SStepInfo()		{disable = true;}
};

enum ELegType {
	eFrontLeft,
	eFrontRight,
	eBackRight,
	eBackLeft
};

