#pragma once

#include "../state.h"
#include "../../../ai_debug.h"

template<typename _Object>
class CStateMonsterRestFun : public CState<_Object> {
	typedef CState<_Object> inherited;

	u32					time_last_hit;

public:
						CStateMonsterRestFun	(_Object *obj);
	virtual	void		initialize				();
	virtual	void		execute					();
	virtual	bool		check_completion		();
	virtual	bool		check_start_conditions	();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_rest_fun_inline.h"
