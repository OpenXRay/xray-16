#pragma once
#include "../monster_state_manager.h"

class CController;

class CStateManagerController : public CMonsterStateManager<CController> {

	typedef CMonsterStateManager<CController> inherited;

public:
						CStateManagerController			(CController *obj);
	virtual				~CStateManagerController		();

	virtual void		reinit							();
	virtual	void		execute							();
	virtual void		remove_links					(CObject* object) { inherited::remove_links(object);}
	virtual bool		check_control_start_conditions	(ControlCom::EControlType type);
};
