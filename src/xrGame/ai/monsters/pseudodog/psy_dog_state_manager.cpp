#include "StdAfx.h"
#include "psy_dog.h"
#include "psy_dog_state_manager.h"
#include "Actor.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"
#include "ai/monsters/control_animation_base.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "sound_player.h"
#include "xrAICore/Navigation/level_graph.h"

#include "psy_dog_state_psy_attack.h"

CStateManagerPsyDog::CStateManagerPsyDog(CAI_PseudoDog* monster) : inherited(monster)
{
    //	add_state(eStateAttack_Psy,	 new CStatePsyDogPsyAttack<CAI_PseudoDog>(monster));
}

void CStateManagerPsyDog::execute()
{
    // 	const CEntityAlive* enemy	= object->EnemyMan.get_enemy();
    //
    // 	if (enemy && smart_cast<const CActor*>(enemy) && smart_cast<CPsyDog*>(object)->must_hide()) {
    //
    // 		select_state(eStateAttack_Psy);
    //
    // 		// выполнить текущее состояние
    // 		get_state_current()->execute();
    //
    // 		prev_substate = current_substate;
    // 	} else {
    inherited::execute();
    //	}
}
