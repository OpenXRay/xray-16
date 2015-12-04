#pragma once
#include "ai/monsters/monster_state_manager.h"

class CFracture;

class CStateManagerFracture : public CMonsterStateManager<CFracture> {
	typedef CMonsterStateManager<CFracture> inherited;

public:
						CStateManagerFracture	(CFracture *obj);
	virtual				~CStateManagerFracture	();

	virtual	void		execute					();
	virtual void		remove_links			(IGameObject* object) { inherited::remove_links(object);}
};
