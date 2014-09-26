#pragma once

#include "../state.h"

template<typename _Object>
class	CStateControllerFastMove : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
public:
						CStateControllerFastMove	(_Object *obj) : inherited(obj) {}
	virtual void		initialize					();	
	virtual void		finalize					();	
	virtual void		critical_finalize			();

	virtual void		execute						();
};

#include "controller_state_attack_fast_move_inline.h"
