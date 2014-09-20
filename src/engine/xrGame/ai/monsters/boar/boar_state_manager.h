#pragma once
#include "../monster_state_manager.h"

class CAI_Boar;

class CStateManagerBoar : public CMonsterStateManager<CAI_Boar> {
	typedef CMonsterStateManager<CAI_Boar> inherited;

public:

					CStateManagerBoar	(CAI_Boar *monster); 

	virtual void	execute				();
	virtual void	remove_links		(CObject* object) { inherited::remove_links(object);}
};
