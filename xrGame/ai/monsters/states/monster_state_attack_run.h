#pragma once

#include "../state.h"

template<typename _Object>
class CStateMonsterAttackRun : public CState<_Object> {
	typedef CState<_Object> inherited;

	TTime				m_time_path_rebuild;

public:
	IC					CStateMonsterAttackRun	(_Object *obj) : inherited(obj) {}

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		finalize				();
	virtual void		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool 		check_completion		();
	virtual bool 		check_start_conditions	();

};

#include "monster_state_attack_run_inline.h"
