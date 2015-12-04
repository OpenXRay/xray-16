#pragma once
#include "ai/monsters/monster_state_manager.h"

class CPseudoGigant;

class CStateManagerGigant : public CMonsterStateManager<CPseudoGigant> {
	typedef CMonsterStateManager<CPseudoGigant> inherited;
public:

					CStateManagerGigant	(CPseudoGigant *monster); 
	virtual void	execute				();
	virtual void	remove_links		(IGameObject* object) { inherited::remove_links(object);}
};
