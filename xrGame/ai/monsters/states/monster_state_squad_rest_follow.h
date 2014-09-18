#pragma once

#include "../state.h"

template<typename _Object>
class	CStateMonsterSquadRestFollow : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;

	Fvector		last_point;

public:
						CStateMonsterSquadRestFollow	(_Object *obj);
	virtual				~CStateMonsterSquadRestFollow	();

	virtual void		initialize						();
	virtual void		reselect_state					();
	virtual void		setup_substates					();
	virtual void		check_force_state				();
	virtual void		remove_links					(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_squad_rest_follow_inline.h"
