#pragma once

#include "../state.h"

template<typename _Object>
class CStateMonsterFindEnemyWalkAround : public CState<_Object> {
	typedef CState<_Object> inherited;

public:
						CStateMonsterFindEnemyWalkAround	(_Object *obj) : inherited(obj) {}
	virtual	void		execute						();
	virtual bool		check_completion			() {return false;}
	virtual void		remove_links				(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_find_enemy_walk_inline.h"
