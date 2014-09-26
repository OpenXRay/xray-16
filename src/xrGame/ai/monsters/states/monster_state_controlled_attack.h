#pragma once

#include "../state.h"
#include "monster_state_attack.h"

template<typename _Object>
class	CStateMonsterControlledAttack : public CStateMonsterAttack<_Object> {
	typedef CStateMonsterAttack<_Object>	inherited;

public:
						CStateMonsterControlledAttack	(_Object *obj);
	virtual void		initialize						();
	virtual	void		execute							();
	virtual void		finalize						();
	virtual void		critical_finalize				();

private:
	const CEntityAlive	*get_enemy						();
};

#include "monster_state_controlled_attack_inline.h"
