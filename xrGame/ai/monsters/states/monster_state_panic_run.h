#pragma once

template<typename _Object>
class CStateMonsterPanicRun : public CState<_Object> {
	typedef CState<_Object> inherited;

public:
						CStateMonsterPanicRun	(_Object *obj) : inherited(obj) {}
	virtual				~CStateMonsterPanicRun	() {}

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool		check_completion		();
};

#include "monster_state_panic_run_inline.h"
