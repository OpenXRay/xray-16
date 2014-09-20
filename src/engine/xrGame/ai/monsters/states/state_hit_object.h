#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterHitObject : public CState<_Object> {
	typedef CState<_Object> inherited;

	xr_vector<CObject*>	m_nearest_objects;
	CPhysicsShellHolder	*target;
	bool				m_hitted;
		
public:
					CStateMonsterHitObject	(_Object *obj) : inherited(obj) {}
	
	virtual	void	initialize				();
	virtual	void	execute					();
	virtual bool	check_start_conditions	();
	virtual bool	check_completion		();
};

#include "state_hit_object_inline.h"