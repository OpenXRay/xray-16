#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterTestState : public CState<_Object> {
	typedef CState<_Object> inherited;
	typedef CState<_Object> *state_ptr;

public:
						CStateMonsterTestState	(_Object *obj);
	virtual	void		reselect_state			();
	virtual	void		setup_substates			();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

template<typename _Object>
class CStateMonsterTestCover : public CState<_Object> {
	typedef CState<_Object> inherited;
	typedef CState<_Object> *state_ptr;

	u32					m_last_node;

public:
						CStateMonsterTestCover	(_Object *obj);
	virtual void		initialize				();	
	virtual void		check_force_state		();
	virtual	void		reselect_state			();
	virtual	void		setup_substates			();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "state_test_state_inline.h"