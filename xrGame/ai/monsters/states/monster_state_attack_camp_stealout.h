#pragma once


template<typename _Object>
class CStateMonsterAttackCampStealOut : public CStateMove<_Object> {
	typedef CStateMove<_Object> inherited;

public:
						CStateMonsterAttackCampStealOut	(_Object *obj);

	virtual	void		execute					();
	virtual bool 		check_completion		();
	virtual bool 		check_start_conditions	();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_attack_camp_stealout_inline.h"
