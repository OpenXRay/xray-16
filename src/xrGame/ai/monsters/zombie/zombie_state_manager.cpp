#include "StdAfx.h"
#include "zombie.h"
#include "zombie_state_manager.h"

#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

#include "ai/monsters/states/monster_state_rest.h"
#include "ai/monsters/states/monster_state_attack.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_hear_int_sound.h"
#include "zombie_state_attack_run.h"
#include "EntityCondition.h"
#include "detail_path_manager.h"
#include "ai/monsters/states/monster_state_controlled.h"
#include "ai/monsters/states/monster_state_help_sound.h"

CStateManagerZombie::CStateManagerZombie(CZombie* obj) : inherited(obj)
{
    add_state(eStateRest, new CStateMonsterRest<CZombie>(obj));
    add_state(eStateAttack, new CStateMonsterAttack<CZombie>(obj, new CStateZombieAttackRun<CZombie>(obj),
                                new CStateMonsterAttackMelee<CZombie>(obj)));
    add_state(eStateEat, new CStateMonsterEat<CZombie>(obj));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CZombie>(obj));
    add_state(eStateControlled, new CStateMonsterControlled<CZombie>(obj));
    add_state(eStateHearHelpSound, new CStateMonsterHearHelpSound<CZombie>(obj));
}

CStateManagerZombie::~CStateManagerZombie() {}
void CStateManagerZombie::execute()
{
    if (object->com_man().ta_is_active())
        return;

    u32 state_id = u32(-1);

    if (!object->is_under_control())
    {
        const CEntityAlive* enemy = object->EnemyMan.get_enemy();

        if (enemy)
        {
            state_id = eStateAttack;
        }
        else if (check_state(eStateHearHelpSound))
        {
            state_id = eStateHearHelpSound;
        }
        else if (object->hear_interesting_sound || object->hear_dangerous_sound)
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
    }
    else
        state_id = eStateControlled;

    // установить текущее состояние
    select_state(state_id);

    // выполнить текущее состояние
    get_state_current()->execute();

    prev_substate = current_substate;
}
