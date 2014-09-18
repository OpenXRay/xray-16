#pragma once
#include "../monster_state_manager.h"

class CAI_PseudoDog;

class CStateManagerPseudodog : public CMonsterStateManager<CAI_PseudoDog> {
	typedef CMonsterStateManager<CAI_PseudoDog> inherited;
	
public:

					CStateManagerPseudodog	(CAI_PseudoDog *monster); 
	virtual void	execute					();
	virtual void	remove_links			(CObject* object) { inherited::remove_links(object);}
};
