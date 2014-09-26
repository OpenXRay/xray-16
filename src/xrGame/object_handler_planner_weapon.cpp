////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_planner_weapon.cpp
//	Created 	: 11.03.2004
//  Modified 	: 01.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler action planner weapon handling
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "object_handler_planner.h"
#include "object_property_evaluators.h"
#include "object_actions.h"
#include "object_handler_space.h"
#include "weapon.h"
#include "object_handler_planner_impl.h"
#include "ai/stalker/ai_stalker.h"

using namespace ObjectHandlerSpace;

void CObjectHandlerPlanner::add_evaluators		(CWeapon *weapon)
{
	u16					id = weapon->ID();
	// dynamic state properties
//.	add_evaluator		(uid(id,eWorldPropertyHidden)			,xr_new<CObjectPropertyEvaluatorState>(weapon,m_object,CWeapon::eHidden));
	add_evaluator		(uid(id,eWorldPropertyHidden)			,xr_new<CObjectPropertyEvaluatorWeaponHidden>(weapon,m_object));
	
	// dynamic member properties
	add_evaluator		(uid(id,eWorldPropertyAimed1)			,xr_new<CObjectPropertyEvaluatorMember>(&m_storage,eWorldPropertyAimed1,true));
	add_evaluator		(uid(id,eWorldPropertyAimed2)			,xr_new<CObjectPropertyEvaluatorMember>(&m_storage,eWorldPropertyAimed2,true));
	add_evaluator		(uid(id,eWorldPropertyStrapped)			,xr_new<CObjectPropertyEvaluatorMember>(&m_storage,eWorldPropertyStrapped,true));
	add_evaluator		(uid(id,eWorldPropertyStrapped2Idle)	,xr_new<CObjectPropertyEvaluatorMember>(&m_storage,eWorldPropertyStrapped2Idle,true));

	// dynamic properties
	add_evaluator		(uid(id,eWorldPropertyAmmo1)			,xr_new<CObjectPropertyEvaluatorAmmo> (weapon,m_object,0));
	add_evaluator		(uid(id,eWorldPropertyAmmo2)			,xr_new<CObjectPropertyEvaluatorAmmo> (weapon,m_object,1));
	add_evaluator		(uid(id,eWorldPropertyEmpty1)			,xr_new<CObjectPropertyEvaluatorEmpty>(weapon,m_object,0));
	add_evaluator		(uid(id,eWorldPropertyEmpty2)			,xr_new<CObjectPropertyEvaluatorEmpty>(weapon,m_object,1));
	add_evaluator		(uid(id,eWorldPropertyFull1)			,xr_new<CObjectPropertyEvaluatorFull> (weapon,m_object,0));
	add_evaluator		(uid(id,eWorldPropertyFull2)			,xr_new<CObjectPropertyEvaluatorFull> (weapon,m_object,1));
	add_evaluator		(uid(id,eWorldPropertyReady1)			,xr_new<CObjectPropertyEvaluatorReady>(weapon,m_object,0));
	add_evaluator		(uid(id,eWorldPropertyReady2)			,xr_new<CObjectPropertyEvaluatorReady>(weapon,m_object,1));
	add_evaluator		(uid(id,eWorldPropertyQueueWait1)		,xr_new<CObjectPropertyEvaluatorQueue>(weapon,m_object,0));
	add_evaluator		(uid(id,eWorldPropertyQueueWait2)		,xr_new<CObjectPropertyEvaluatorQueue>(weapon,m_object,1));
	
	// temporary const properties
	add_evaluator		(uid(id,eWorldPropertySwitch1)			,xr_new<CObjectPropertyEvaluatorConst>(true));
	add_evaluator		(uid(id,eWorldPropertySwitch2)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	
	// const properties
	add_evaluator		(uid(id,eWorldPropertyFiring1)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyFiringNoReload1)	,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyFiring2)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyIdle)				,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyIdleStrap)		,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyDropped)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAiming1)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAiming2)			,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAimingReady1)		,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAimingReady2)		,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAimForceFull1)	,xr_new<CObjectPropertyEvaluatorConst>(false));
	add_evaluator		(uid(id,eWorldPropertyAimForceFull2)	,xr_new<CObjectPropertyEvaluatorConst>(false));
}

void CObjectHandlerPlanner::add_operators		(CWeapon *weapon)
{
	u16					id = weapon->ID(), ff = 0xffff;
	CActionBase<CAI_Stalker>	*action;
	
	// show
	action				= xr_new<CObjectActionShow>(weapon,m_object,&m_storage,"show");
	add_condition		(action,id,eWorldPropertyHidden,	true);
	add_condition		(action,ff,eWorldPropertyItemID,	true);
	add_effect			(action,ff,eWorldPropertyItemID,	false);
	add_effect			(action,id,eWorldPropertyHidden,	false);
	add_operator		(uid(id,eWorldOperatorShow),		action);

	// hide
	action				= xr_new<CObjectActionHide>(weapon,m_object,&m_storage,"hide");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,ff,eWorldPropertyItemID,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,ff,eWorldPropertyItemID,	true);
	add_effect			(action,id,eWorldPropertyHidden,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorHide),		action);

	// drop
	action				= xr_new<CObjectActionDrop>(weapon,m_object,&m_storage,"drop");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyDropped,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorDrop),		action);

	// idle
	action				= xr_new<CSObjectActionBase>(weapon,m_object,&m_storage,"idle");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyIdle,		true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorIdle),		action);

	// strapping
	action				= xr_new<CObjectActionStrapping>(weapon,m_object,&m_storage,"strapping");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_effect			(action,id,eWorldPropertyStrapped2Idle,	true);
	add_effect			(action,id,eWorldPropertyStrapped,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorStrapping),	action);

	action				= xr_new<CObjectActionStrappingToIdle>(weapon,m_object,&m_storage,"strapping to idle");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	true);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	true);
	add_effect			(action,id,eWorldPropertyStrapped2Idle,	false);
	add_operator		(uid(id,eWorldOperatorStrapping2Idle),	action);

	// unstrapping
	action				= xr_new<CObjectActionUnstrapping>(weapon,m_object,&m_storage,"unstrapping");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	true);
	add_effect			(action,id,eWorldPropertyStrapped,	false);
	add_effect			(action,id,eWorldPropertyStrapped2Idle,	true);
	add_operator		(uid(id,eWorldOperatorUnstrapping),	action);

	action				= xr_new<CObjectActionUnstrappingToIdle>(weapon,m_object,&m_storage,"unstrapping to idle");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	true);
	add_effect			(action,id,eWorldPropertyStrapped2Idle,	false);
	add_operator		(uid(id,eWorldOperatorUnstrapping2Idle),action);

	// strapped
	action				= xr_new<CSObjectActionBase>(weapon,m_object,&m_storage,"strapped");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	true);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_condition		(action,id,eWorldPropertyIdleStrap,	false);
	add_effect			(action,id,eWorldPropertyIdleStrap,	true);
	add_operator		(uid(id,eWorldOperatorStrapped),	action);

	// aim1
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed1,true,"aim1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed1,	true);
	add_effect			(action,id,eWorldPropertyAiming1,	true);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorAim1),		action);

	// aim2
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed2,true,"aim2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch2,	true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	true);
	add_effect			(action,id,eWorldPropertyAiming2,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_operator		(uid(id,eWorldOperatorAim2),		action);

	// aim_queue1
	action				= xr_new<CObjectActionQueueWait>(weapon,m_object,&m_storage,uid(id,eWorldPropertyQueueWait1),"aim_queue1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyQueueWait1,false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyQueueWait1,true);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorQueueWait1),	action);

	// aim_queue2
	action				= xr_new<CObjectActionQueueWait>(weapon,m_object,&m_storage,uid(id,eWorldPropertyQueueWait2),"aim_queue2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyQueueWait2,false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyQueueWait2,true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_operator		(uid(id,eWorldOperatorQueueWait2),	action);

	// fire1
	action				= xr_new<CObjectActionFire>(weapon,m_object,&m_storage,uid(id,eWorldPropertyQueueWait1),"fire1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyReady1,	true);
	add_condition		(action,id,eWorldPropertyEmpty1,	false);
	add_condition		(action,id,eWorldPropertyAimed1,	true);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyQueueWait1,true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyFiring1,	true);
	add_operator		(uid(id,eWorldOperatorFire1),		action);

	// fire no reload
	action				= xr_new<CObjectActionFireNoReload>(weapon,m_object,&m_storage,uid(id,eWorldPropertyQueueWait1),"fire_no_reload");
	add_condition		(action,id,eWorldPropertyHidden,	false);
//	add_condition		(action,id,eWorldPropertyEmpty1,	false);
//	add_condition		(action,id,eWorldPropertyAimed1,	true);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
//	add_condition		(action,id,eWorldPropertyQueueWait1,true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyFiringNoReload1,	true);
	add_operator		(uid(id,eWorldOperatorFireNoReload),action);

	// fire2
	action				= xr_new<CObjectActionFire>(weapon,m_object,&m_storage,uid(id,eWorldPropertyQueueWait2),"fire2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyReady2,	true);
	add_condition		(action,id,eWorldPropertyEmpty2,	false);
	add_condition		(action,id,eWorldPropertyAimed2,	true);
	add_condition		(action,id,eWorldPropertySwitch2,	true);
	add_condition		(action,id,eWorldPropertyQueueWait2,true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyFiring2,	true);
	add_operator		(uid(id,eWorldOperatorFire2),		action);

	// reload1
	action				= xr_new<CObjectActionReload>(weapon,m_object,&m_storage,0,"reload1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyReady1,	false);
	add_condition		(action,id,eWorldPropertyAmmo1,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyEmpty1,	false);
	add_effect			(action,id,eWorldPropertyReady1,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorReload1),		action);

	// reload2
	action				= xr_new<CObjectActionReload>(weapon,m_object,&m_storage,1,"reload2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyReady2,	false);
	add_condition		(action,id,eWorldPropertyAmmo2,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyEmpty2,	false);
	add_effect			(action,id,eWorldPropertyReady2,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorReload2),		action);

	// force_reload1
	action				= xr_new<CObjectActionReload>(weapon,m_object,&m_storage,0,"force_reload1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyFull1,		false);
	add_condition		(action,id,eWorldPropertyAmmo1,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyEmpty1,	false);
	add_effect			(action,id,eWorldPropertyReady1,	true);
	add_effect			(action,id,eWorldPropertyFull1,		true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorForceReload1),action);

	// force_reload2
	action				= xr_new<CObjectActionReload>(weapon,m_object,&m_storage,0,"force_reload2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyFull2,		false);
	add_condition		(action,id,eWorldPropertyAmmo2,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyEmpty2,	false);
	add_effect			(action,id,eWorldPropertyReady2,	true);
	add_effect			(action,id,eWorldPropertyFull2,		true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorForceReload2),action);

	// switch1
	action				= xr_new<CObjectActionSwitch>(weapon,m_object,&m_storage,0,"switch1");
	add_condition		(action,id,eWorldPropertySwitch1,	false);
	add_condition		(action,id,eWorldPropertySwitch2,	true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertySwitch1,	true);
	add_effect			(action,id,eWorldPropertySwitch2,	false);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorSwitch1),		action);

	// switch2
	action				= xr_new<CObjectActionSwitch>(weapon,m_object,&m_storage,1,"switch2");
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertySwitch2,	false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertySwitch1,	false);
	add_effect			(action,id,eWorldPropertySwitch2,	true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorSwitch2),		action);

	// aiming ready1
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed1,true,"aim_ready1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyReady1,	true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed1,	true);
	add_effect			(action,id,eWorldPropertyAimingReady1,true);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorAimingReady1),action);

	// aiming ready2
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed2,true,"aim_ready2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch2,	true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	true);
	add_effect			(action,id,eWorldPropertyAimingReady2,true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_operator		(uid(id,eWorldOperatorAimingReady2),action);

	// force aim full 1
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed1,true,"aim_ready1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch1,	true);
	add_condition		(action,id,eWorldPropertyReady1,	true);
	add_condition		(action,id,eWorldPropertyFull1,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed1,	true);
	add_effect			(action,id,eWorldPropertyAimForceFull1,	true);
//	add_effect			(action,id,eWorldPropertyAimingReady1,true);
	add_effect			(action,id,eWorldPropertyAimed2,	false);
	add_operator		(uid(id,eWorldOperatorAimForceFull1),action);

	// force aim full 2
	action				= xr_new<CObjectActionAim>(weapon,m_object,&m_storage,eWorldPropertyAimed2,true,"aim_ready2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertySwitch2,	true);
	add_condition		(action,id,eWorldPropertyReady2,	true);
	add_condition		(action,id,eWorldPropertyFull2,		true);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAimed2,	true);
	add_effect			(action,id,eWorldPropertyAimForceFull2,	true);
//	add_effect			(action,id,eWorldPropertyAimingReady2,true);
	add_effect			(action,id,eWorldPropertyAimed1,	false);
	add_operator		(uid(id,eWorldOperatorAimForceFull2),action);

	// fake action get ammo
	action				= xr_new<CSObjectActionBase>(weapon,m_object,&m_storage,"fake_get_ammo1");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyAmmo1,		false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAmmo1,		true);
	add_operator		(uid(id,eWorldOperatorGetAmmo1),	action);

	action				= xr_new<CSObjectActionBase>(weapon,m_object,&m_storage,"fake_get_ammo2");
	add_condition		(action,id,eWorldPropertyHidden,	false);
	add_condition		(action,id,eWorldPropertyAmmo2,		false);
	add_condition		(action,id,eWorldPropertyStrapped,	false);
	add_condition		(action,id,eWorldPropertyStrapped2Idle,	false);
	add_effect			(action,id,eWorldPropertyAmmo2,		true);
	add_operator		(uid(id,eWorldOperatorGetAmmo2),	action);

	this->action(uid(id,eWorldOperatorAim1)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorAim2)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorAimingReady1)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorAimingReady2)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorAimForceFull1)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorAimForceFull2)).set_inertia_time(500);
	this->action(uid(id,eWorldOperatorQueueWait1)).set_inertia_time(300);
	this->action(uid(id,eWorldOperatorQueueWait2)).set_inertia_time(300);
}
