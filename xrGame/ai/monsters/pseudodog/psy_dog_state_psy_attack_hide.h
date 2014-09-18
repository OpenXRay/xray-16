#pragma once

template<typename _Object>
class CStatePsyDogHide : public CState<_Object> {
	typedef	CState<_Object>		inherited;
	typedef	CState<_Object>*	state_ptr;

	struct {
		Fvector position;
		u32		node;
	} target;


public:
					CStatePsyDogHide		(_Object *obj) : inherited(obj) {}
	virtual			~CStatePsyDogHide		() {}

	virtual void	initialize				();
	virtual void	execute					();
	virtual void	remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool 	check_completion		();
	virtual bool 	check_start_conditions	();

private:
			void	select_target_point		();
};

#include "psy_dog_state_psy_attack_hide_inline.h"

