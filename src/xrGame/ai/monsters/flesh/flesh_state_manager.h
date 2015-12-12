#pragma once
#include "ai/monsters/monster_state_manager.h"

class CAI_Flesh;

class CStateManagerFlesh : public CMonsterStateManager<CAI_Flesh> {
	typedef CMonsterStateManager<CAI_Flesh> inherited;

public:

					CStateManagerFlesh	(CAI_Flesh *monster); 
	virtual void	execute				();
	virtual void	remove_links		(IGameObject* object) { inherited::remove_links(object);}
};
