#pragma once

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
};
