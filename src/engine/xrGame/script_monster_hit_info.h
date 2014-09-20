#pragma once

#include "script_export_space.h"

class CScriptGameObject;

class CScriptMonsterHitInfo {
public:
	CScriptGameObject			*who;
	Fvector					direction;
	int						time;		


	CScriptMonsterHitInfo		()
	{
		who				= 0;
		time			= 0;
		direction		= Fvector().set(0.f,0.f,1.f);
	}

	void set(CScriptGameObject *p_who, Fvector p_direction, int p_time) {
		who			= p_who;
		direction	= p_direction;
		time		= p_time;
	}
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptMonsterHitInfo)
#undef script_type_list
#define script_type_list save_type_list(CScriptMonsterHitInfo)
