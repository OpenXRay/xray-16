#pragma once
#include "../monster_state_manager.h"

class CChimera;

class CStateManagerChimera : public CMonsterStateManager<CChimera>
{
private:
	typedef				CMonsterStateManager<CChimera>	inherited;

public:
						CStateManagerChimera	(CChimera *obj);
	virtual				~CStateManagerChimera	();

	virtual	void		execute					();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};
