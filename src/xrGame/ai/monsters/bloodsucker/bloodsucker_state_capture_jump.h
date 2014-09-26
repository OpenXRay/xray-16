#pragma once

#include "../state.h"
//#include "../../../entitycondition.h"

template<typename _Object>
class	CStateCaptureJumpBloodsucker : public CState<_Object> {
protected:
	typedef CState<_Object>		inherited;
	typedef CState<_Object>*	state_ptr;

public:
	CStateCaptureJumpBloodsucker		(_Object *obj);
	virtual				~CStateCaptureJumpBloodsucker		();

	virtual	void		execute					();
	virtual void		setup_substates			();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}
};

#include "bloodsucker_state_capture_jump_inline.h"