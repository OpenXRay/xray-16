#include "StdAfx.h"
#include "ai/monsters/pseudodog/pseudodog.h"
#include "ai/monsters/pseudodog/pseudodog_state_manager.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"
#include "Actor.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/monsters/states/monster_state_rest.h"
#include "ai/monsters/states/monster_state_attack.h"
#include "ai/monsters/states/monster_state_panic.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_hear_int_sound.h"
#include "ai/monsters/states/monster_state_hear_danger_sound.h"
#include "ai/monsters/states/monster_state_hitted.h"

CStateManagerPseudodog::CStateManagerPseudodog(CAI_PseudoDog* monster) : inherited(monster)
{
    add_state(eStateRest, new CStateMonsterRest<CAI_PseudoDog>(monster));
    add_state(eStatePanic, new CStateMonsterPanic<CAI_PseudoDog>(monster));

    CStateMonsterAttackMoveToHomePoint<CAI_PseudoDog>* move2home =
        new CStateMonsterAttackMoveToHomePoint<CAI_PseudoDog>(monster);

    add_state(eStateAttack, new CStateMonsterAttack<CAI_PseudoDog>(monster, move2home));

    add_state(eStateEat, new CStateMonsterEat<CAI_PseudoDog>(monster));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CAI_PseudoDog>(monster));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CAI_PseudoDog>(monster));
    add_state(eStateHitted, new CStateMonsterHitted<CAI_PseudoDog>(monster));
}

#define MIN_ANGRY_TIME 10000
#define MAX_GROWLING_TIME 20000

void CStateManagerPseudodog::execute()
{
    u32 state_id = u32(-1);

    const CEntityAlive* enemy = object->EnemyMan.get_enemy();

    if (enemy)
    {
        switch (object->EnemyMan.get_danger_type())
        {
        case eStrong: state_id = eStatePanic; break;
        case eWeak: state_id = eStateAttack; break;
        }
    }
    else if (object->HitMemory.is_hit())
    {
        state_id = eStateHitted;
    }
    else if (object->hear_interesting_sound)
    {
        state_id = eStateHearInterestingSound;
    }
    else if (object->hear_dangerous_sound)
    {
        state_id = eStateHearDangerousSound;
    }
    else
    {
        if (can_eat())
            state_id = eStateEat;
        else
            state_id = eStateRest;
    }

    select_state(state_id);

    // выполнить текущее состояние
    get_state_current()->execute();

    prev_substate = current_substate;
}
