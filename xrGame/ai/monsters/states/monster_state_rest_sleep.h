#pragma once

#include "../state.h"
#include "../../../ai_debug.h"

template<typename _Object>
class CStateMonsterRestSleep : public CState<_Object> {
	typedef CState<_Object> inherited;
public:
						CStateMonsterRestSleep	(_Object *obj);
	virtual				~CStateMonsterRestSleep	();

	virtual	void		initialize				();
	virtual	void		execute					();
	virtual	void		finalize				();
	virtual	void		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_rest_sleep_inline.h"
