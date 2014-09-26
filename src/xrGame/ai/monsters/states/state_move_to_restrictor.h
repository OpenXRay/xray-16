#pragma once
#include "../state.h"

template<typename _Object>
class CStateMonsterMoveToRestrictor : public CState<_Object> {
	typedef CState<_Object> inherited;

public:
						CStateMonsterMoveToRestrictor	(_Object *obj) : inherited(obj) {}
	virtual				~CStateMonsterMoveToRestrictor	() {}

	virtual void		initialize					();
	virtual	void		execute						();

	virtual bool		check_start_conditions		();
	virtual bool		check_completion			();
	virtual void		remove_links				(CObject* object) { inherited::remove_links(object);}
};

#include "state_move_to_restrictor_inline.h"
