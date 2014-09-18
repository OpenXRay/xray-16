#pragma once

template<typename _Object>
class CStateGroupPanicRun : public CState<_Object> {
	typedef CState<_Object> inherited;

public:
						CStateGroupPanicRun	(_Object *obj) : inherited(obj) {}
	virtual				~CStateGroupPanicRun	() {}

	virtual void		initialize				();
	virtual	void		execute					();

	virtual bool		check_completion		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "group_state_panic_run_inline.h"
