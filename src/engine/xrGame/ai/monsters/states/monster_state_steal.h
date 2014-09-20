#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterSteal : public CState<_Object> {
	typedef CState<_Object> inherited;

public:
						CStateMonsterSteal		(_Object *obj);

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool 		check_completion		();
	virtual bool 		check_start_conditions	();

private:
			bool		check_conditions		();
};

#include "monster_state_steal_inline.h"
