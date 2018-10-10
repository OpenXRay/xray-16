#include "StdAfx.h"
#include "fracture.h"
#include "fracture_state_manager.h"

#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

#include "ai/monsters/states/monster_state_rest.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_attack.h"
#include "ai/monsters/states/monster_state_panic.h"
#include "ai/monsters/states/monster_state_hear_danger_sound.h"
#include "ai/monsters/states/monster_state_hitted.h"

#include "EntityCondition.h"

CStateManagerFracture::CStateManagerFracture(CFracture* obj) : inherited(obj)
{
    add_state(eStateRest, new CStateMonsterRest<CFracture>(obj));
    add_state(eStateAttack, new CStateMonsterAttack<CFracture>(obj));
    add_state(eStateEat, new CStateMonsterEat<CFracture>(obj));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CFracture>(obj));
    add_state(eStatePanic, new CStateMonsterPanic<CFracture>(obj));
    add_state(eStateHitted, new CStateMonsterHitted<CFracture>(obj));
}

CStateManagerFracture::~CStateManagerFracture() {}
void CStateManagerFracture::execute()
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
    else if (object->hear_interesting_sound || object->hear_dangerous_sound)
    {
        state_id = eStateHearDangerousSound;
    }
    else
    {
        if (can_eat())
            state_id = eStateEat;
        else
        {
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
