#pragma once
#include "ai/Monsters/monster_state_manager.h"

class CSnork;

class CStateManagerSnork : public CMonsterStateManager<CSnork> 
{
private:
	typedef				CMonsterStateManager<CSnork>	inherited;

public:
						CStateManagerSnork		(CSnork *obj);
	virtual				~CStateManagerSnork		();

	virtual	void		execute					();
	virtual void		remove_links			(IGameObject* object) { inherited::remove_links(object);}
};
