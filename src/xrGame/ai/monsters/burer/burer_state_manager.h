#pragma once
#include "ai/monsters/monster_state_manager.h"

class CBurer;

class CStateManagerBurer : public CMonsterStateManager<CBurer> {
	typedef CMonsterStateManager<CBurer> inherited;
public:
					CStateManagerBurer		(CBurer *monster); 
	virtual void	execute					();
	virtual void	setup_substates			();
	virtual void	remove_links			(IGameObject* object) { inherited::remove_links(object);}
};

