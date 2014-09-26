#pragma once

template<typename _Object>
class CStateControlAttack : public CState<_Object> {
	typedef	CState<_Object> inherited;

	enum {
		eActionPrepare,
		eActionContinue,
		eActionFire,
		eActionWaitTripleEnd,
		eActionCompleted
	} m_action;

	u32				time_control_started;

public:

					CStateControlAttack		(_Object *p);
	virtual			~CStateControlAttack	();

	virtual void	initialize				();	
	virtual void	execute					();
	virtual void	finalize				();
	virtual void	critical_finalize		();

	virtual bool 	check_completion		();
	virtual bool 	check_start_conditions	();

private:

			void	execute_hit_fire		();
			void	execute_hit_continue	();
			void	execute_hit_prepare		();
};

#include "controller_state_control_hit_inline.h"