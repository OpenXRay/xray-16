#pragma once

#include "../state.h"
#include "../../../entitycondition.h"

template<typename _Object>
class	CStateMonsterRest : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;

	u32					time_last_fun;	
	u32					time_idle_selected;

public:
						CStateMonsterRest		(_Object *obj);
	virtual				~CStateMonsterRest		();

	virtual	void		initialize				();
	virtual	void		execute					();
	virtual	void		finalize				();
	virtual	void		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_rest_inline.h"
