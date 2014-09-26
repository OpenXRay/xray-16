#include "stdafx.h"
#include "controller.h"
#include "controller_state_manager.h"

#include "controller_animation.h"
#include "controller_direction.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../controlled_entity.h"

#include "../states/monster_state_rest.h"
#include "controller_state_attack.h"
#include "../states/monster_state_attack_melee.h"
#include "../states/monster_state_attack_run.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_hear_int_sound.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"
#include "../states/monster_state_attack.h"

#include "../../../entitycondition.h"

#include "../states/state_test_state.h"

CStateManagerController::CStateManagerController(CController *obj) : inherited(obj)
{
	add_state(eStateRest,					xr_new<CStateMonsterRest<CController> >					(obj));
	add_state(eStatePanic,					xr_new<CStateMonsterPanic<CController> >				(obj));
	add_state(eStateHearInterestingSound,	xr_new<CStateMonsterHearInterestingSound<CController> >	(obj));
	add_state(eStateHearDangerousSound,		xr_new<CStateMonsterHearDangerousSound<CController> >	(obj));
	add_state(eStateHitted,					xr_new<CStateMonsterHitted<CController> >				(obj));
	add_state(eStateAttack,					xr_new<CStateControllerAttack<CController> >			(obj));

	add_state(eStateEat,		xr_new<CStateMonsterEat<CController> >(obj));
	add_state(eStateCustom,		xr_new<CStateControlHide<CController> >(obj));
}

bool   CStateManagerController::check_control_start_conditions	(ControlCom::EControlType type)
{
	if ( type == ControlCom::eAntiAim )
	{		
		return current_substate == eStateAttack_Run;
	}

	return false;
}

CStateManagerController::~CStateManagerController()
{
}

void CStateManagerController::reinit()
{
	inherited::reinit();
	object->set_mental_state(CController::eStateIdle);
}


#define FIND_ENEMY_TIME_ENEMY_HIDDEN	5000
#define FIND_ENEMY_MAX_DISTANCE			10.f

void CStateManagerController::execute()
{
	u32 state_id = u32(-1);
		
	const CEntityAlive* enemy	= object->EnemyMan.get_enemy();

	if (enemy) {
		state_id = eStateAttack;
	} else if (object->HitMemory.is_hit()) {
		state_id = eStateHitted;
	} else if (object->hear_interesting_sound) {
		state_id = eStateHearInterestingSound;
	} else if (object->hear_dangerous_sound) {
		state_id = eStateHearDangerousSound;	
	} else {
		if (can_eat())	state_id = eStateEat;
		else			state_id = eStateRest;
	}

	select_state(state_id); 

	// выполнить текущее состояние
	get_state_current()->execute();

	prev_substate = current_substate;
}
