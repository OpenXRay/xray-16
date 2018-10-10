#include "StdAfx.h"
#include "cat.h"
#include "cat_state_manager.h"

#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

#include "ai/monsters/states/monster_state_rest.h"
#include "ai/monsters/states/monster_state_attack.h"
#include "ai/monsters/states/monster_state_panic.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_hear_int_sound.h"
#include "ai/monsters/states/monster_state_hear_danger_sound.h"
#include "ai/monsters/states/monster_state_hitted.h"
#include "ai/monsters/states/state_test_look_actor.h"
#include "EntityCondition.h"
#include "ai/monsters/states/monster_state_help_sound.h"

CStateManagerCat::CStateManagerCat(CCat* obj) : inherited(obj)
{
    add_state(eStateRest, new CStateMonsterRest<CCat>(obj));
    add_state(eStatePanic, new CStateMonsterPanic<CCat>(obj));
    add_state(eStateAttack, new CStateMonsterAttack<CCat>(obj));
    add_state(eStateEat, new CStateMonsterEat<CCat>(obj));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CCat>(obj));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CCat>(obj));
    add_state(eStateHitted, new CStateMonsterHitted<CCat>(obj));

    add_state(eStateThreaten, new CStateMonsterLookActor<CCat>(obj));
    add_state(eStateHearHelpSound, new CStateMonsterHearHelpSound<CCat>(obj));

    m_rot_jump_last_time = 0;
}

CStateManagerCat::~CStateManagerCat() {}
#define ROTATION_JUMP_DELAY 3000

void CStateManagerCat::execute()
{
    u32 state_id = u32(-1);

    const CEntityAlive* enemy = object->EnemyMan.get_enemy();

    if (enemy)
    {
        {
            switch (object->EnemyMan.get_danger_type())
            {
            case eStrong: state_id = eStatePanic; break;
            case eWeak: state_id = eStateAttack; break;
            }
        }
    }
    else if (object->HitMemory.is_hit())
    {
        state_id = eStateHitted;
    }
    else if (check_state(eStateHearHelpSound))
    {
        state_id = eStateHearHelpSound;
    }
    else if (object->hear_dangerous_sound)
    {
        state_id = eStateHearDangerousSound;
    }
    else if (object->hear_interesting_sound)
    {
        state_id = eStateHearInterestingSound;
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
