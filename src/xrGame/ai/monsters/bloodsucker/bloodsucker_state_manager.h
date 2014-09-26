#pragma once
#include "../monster_state_manager.h"

class CAI_Bloodsucker;

class CStateManagerBloodsucker : public CMonsterStateManager<CAI_Bloodsucker> {
	typedef CMonsterStateManager<CAI_Bloodsucker> inherited;

public:
					CStateManagerBloodsucker	(CAI_Bloodsucker *monster); 
	virtual void	execute						();
	virtual void	update						();
			void	drag_object					();
	virtual void	remove_links				(CObject* object) { inherited::remove_links(object);}
			bool	check_vampire				();
};
