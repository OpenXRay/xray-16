#pragma once
#include "../state.h"

template<typename _Object>
class CStateControllerTube : public CState<_Object> {
	typedef CState<_Object>		inherited;

public:
						CStateControllerTube	(_Object *obj) : inherited(obj){}
	virtual void		execute					();
	virtual bool		check_start_conditions	();
	virtual bool		check_completion		();
};

#include "controller_tube_inline.h"

