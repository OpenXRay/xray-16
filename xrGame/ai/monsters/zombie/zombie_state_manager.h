#pragma once
#include "../monster_state_manager.h"

class CZombie;

class CStateManagerZombie : public CMonsterStateManager<CZombie> {
	typedef CMonsterStateManager<CZombie> inherited;

public:
						CStateManagerZombie		(CZombie *obj);
	virtual				~CStateManagerZombie	();

	virtual	void		execute					();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};
