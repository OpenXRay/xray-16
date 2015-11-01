#pragma once
#include "ai/Monsters/BaseMonster/base_monster.h"


class CStateManagerFracture;

class CFracture : public CBaseMonster {
	typedef		CBaseMonster		inherited;
	
public:
					CFracture 			();
	virtual			~CFracture 			();	

	virtual void	Load				(LPCSTR section);
	virtual void	CheckSpecParams		(u32 spec_params);

	virtual	char*	get_monster_class_name () { return "fracture"; }
};
