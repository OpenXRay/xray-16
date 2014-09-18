#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterEating : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;

	CEntityAlive	*corpse;
	u32				time_last_eat;

public:
						CStateMonsterEating		(_Object *obj);
	virtual				~CStateMonsterEating	();

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		remove_links			(CObject* object);

	virtual bool		check_start_conditions	();
	virtual bool		check_completion		();
};

#include "monster_state_eat_eat_inline.h"
