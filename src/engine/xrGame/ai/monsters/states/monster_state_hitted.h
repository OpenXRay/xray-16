#pragma once

#include "../state.h"

template<typename _Object>
class	CStateMonsterHitted : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;

public:
					CStateMonsterHitted		(_Object *obj);
	virtual			~CStateMonsterHitted	();

	virtual	void	reselect_state			();
	virtual void	remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_hitted_inline.h"
