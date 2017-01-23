#pragma once
#include "ai/Monsters/BaseMonster/base_monster.h"
#include "ai/Monsters/controlled_entity.h"


class CTushkano :	public CBaseMonster,
					public CControlledEntity<CTushkano> {


	typedef		CBaseMonster					inherited;
	typedef		CControlledEntity<CTushkano>	CControlled;

public:
					CTushkano 			();
	virtual			~CTushkano 			();	

	virtual void	Load				(LPCSTR section);
	virtual void	CheckSpecParams		(u32 spec_params);
	virtual	char*	get_monster_class_name () { return "tushkano"; }
};
