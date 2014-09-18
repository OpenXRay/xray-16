#pragma once

#include "../state.h"

template<typename _Object>
class CStateGroupRestIdle : public CState<_Object> {
	typedef CState<_Object> inherited;
	typedef CState<_Object> *state_ptr;

	u32					m_target_node;
	int					m_move_type;

public:
						CStateGroupRestIdle	(_Object *obj);
	virtual void 		initialize				();
	virtual void 		finalize				();
	virtual void 		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual	void		reselect_state			();
	virtual	void		setup_substates			();
};

#include "group_state_rest_idle_inline.h"
