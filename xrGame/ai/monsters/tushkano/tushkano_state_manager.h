#pragma once
#include "../monster_state_manager.h"

class CTushkano;

class CStateManagerTushkano : public CMonsterStateManager<CTushkano> {
	typedef CMonsterStateManager<CTushkano> inherited;

public:
						CStateManagerTushkano	(CTushkano *obj);
	virtual				~CStateManagerTushkano	();

	virtual	void		execute					();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};
