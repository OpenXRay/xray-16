#pragma once

#include "script_export_space.h"

class CScriptGameObject;

class CScriptSoundInfo {
public:
	CScriptGameObject			*who;
	Fvector					position;
	float					power;
	int						time;		
	int						dangerous;


	CScriptSoundInfo				()
	{
		who				= 0;
		time			= 0;
		dangerous		= 0;
		power			= 0.f;
		position		= Fvector().set(0.f,0.f,0.f);
	}

	void set(CScriptGameObject *p_who, bool p_danger, Fvector p_position, float p_power, int p_time) {
		who			= p_who;
		position	= p_position;
		power		= p_power;
		time		= p_time;
		dangerous	= int(p_danger);
	}
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptSoundInfo)
#undef script_type_list
#define script_type_list save_type_list(CScriptSoundInfo)
