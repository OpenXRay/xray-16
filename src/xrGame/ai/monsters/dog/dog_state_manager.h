#pragma once
#include "ai/monsters/monster_state_manager.h"

class CAI_Dog;

class CStateManagerDog : public CMonsterStateManager<CAI_Dog> {
	typedef CMonsterStateManager<CAI_Dog> inherited;

public:

					CStateManagerDog	(CAI_Dog *monster); 
	virtual void	execute				();
			bool	check_eat			();
	virtual void	remove_links		(IGameObject* object) { inherited::remove_links(object);}
};
