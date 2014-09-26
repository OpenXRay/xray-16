#include "stdafx.h"
#include "fracture.h"
#include "fracture_state_manager.h"

#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../control_path_builder_base.h"

#include "../states/monster_state_rest.h"
#include "../states/monster_state_eat.h"
#include "../states/monster_state_attack.h"
#include "../states/monster_state_panic.h"
#include "../states/monster_state_hear_danger_sound.h"
#include "../states/monster_state_hitted.h"

#include "../../../entitycondition.h"

CStateManagerFracture::CStateManagerFracture(CFracture *obj) : inherited(obj)
{
	add_state(eStateRest,					xr_new<CStateMonsterRest<CFracture> >				(obj));
	add_state(eStateAttack,					xr_new<CStateMonsterAttack<CFracture> >				(obj));
	add_state(eStateEat,					xr_new<CStateMonsterEat<CFracture> >				(obj));
	add_state(eStateHearDangerousSound,		xr_new<CStateMonsterHearDangerousSound<CFracture> >	(obj));
	add_state(eStatePanic,					xr_new<CStateMonsterPanic<CFracture> >				(obj));
	add_state(eStateHitted,					xr_new<CStateMonsterHitted<CFracture> >				(obj));
}

CStateManagerFracture::~CStateManagerFracture()
{
}

void CStateManagerFracture::execute()
{
	u32 state_id = u32(-1);
	const CEntityAlive* enemy	= object->EnemyMan.get_enemy();

	if (enemy) {
		switch (object->EnemyMan.get_danger_type()) {
				case eStrong:	state_id = eStatePanic; break;
				case eWeak:		state_id = eStateAttack; break;
		}
	} else if (object->HitMemory.is_hit()) {
		state_id = eStateHitted;
	} else if (object->hear_interesting_sound || object->hear_dangerous_sound) {
		state_id = eStateHearDangerousSound;
	} else {
		if (can_eat()) state_id = eStateEat;
		else {
			// Rest & Idle states here 
			state_id = eStateRest;
		}
	}

	// установить текущее состояние
	select_state(state_id); 

	// выполнить текущее состояние
	get_state_current()->execute();

	prev_substate = current_substate;
}

