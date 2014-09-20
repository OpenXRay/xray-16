#pragma once
#include "../state.h"

#include "../../../alife_smart_terrain_task.h"

template<typename _Object>
class CStateMonsterSmartTerrainTask : public CState<_Object> {
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;

	CALifeSmartTerrainTask *m_current_task;
public:
						CStateMonsterSmartTerrainTask	(_Object *obj);
	virtual				~CStateMonsterSmartTerrainTask	();

	virtual void		initialize				();
	virtual	void		reselect_state			();
	virtual bool		check_start_conditions	();
	virtual bool		check_completion		();
	virtual void		setup_substates			();
	virtual void		check_force_state		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "monster_state_smart_terrain_task_inline.h"
