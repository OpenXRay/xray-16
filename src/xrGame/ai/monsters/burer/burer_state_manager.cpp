#include "StdAfx.h"
#include "burer.h"
#include "burer_state_manager.h"

#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

#include "ai/monsters/states/monster_state_rest.h"
#include "ai/monsters/states/monster_state_panic.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_hear_int_sound.h"
#include "ai/monsters/states/monster_state_hear_danger_sound.h"
#include "ai/monsters/states/monster_state_hitted.h"
#include "ai/monsters/states/state_custom_action.h"

#include "burer_state_attack.h"

CStateManagerBurer::CStateManagerBurer(CBurer* monster) : inherited(monster)
{
    add_state(eStateRest, new CStateMonsterRest<CBurer>(monster));
    add_state(eStatePanic, new CStateMonsterPanic<CBurer>(monster));
    add_state(eStateAttack, new CStateBurerAttack<CBurer>(monster));
    add_state(eStateEat, new CStateMonsterEat<CBurer>(monster));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CBurer>(monster));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CBurer>(monster));
    add_state(eStateHitted, new CStateMonsterHitted<CBurer>(monster));
    add_state(eStateBurerScanning, new CStateMonsterCustomAction<CBurer>(monster));
}

#define SCAN_STATE_TIME 4000

void CStateManagerBurer::execute()
{
    u32 state = u32(-1);

    if (object->EnemyMan.get_enemy())
    {
        switch (object->EnemyMan.get_danger_type())
        {
        case eStrong: state = eStatePanic; break;
        case eWeak: state = eStateAttack; break;
        }
    }
    else if (object->HitMemory.is_hit() && (object->HitMemory.get_last_hit_time() + 10000 > Device.dwTimeGlobal))
        state = eStateHitted;
    else if (object->hear_interesting_sound)
        state = eStateHearInterestingSound;
    else if (object->hear_dangerous_sound)
        state = eStateHearDangerousSound;
    else if (object->time_last_scan + SCAN_STATE_TIME > Device.dwTimeGlobal)
        state = eStateBurerScanning;
    else if (can_eat())
        state = eStateEat;
    else
        state = eStateRest;

    select_state(state);

    // выполнить текущее состояние
    get_state_current()->execute();

    prev_substate = current_substate;
}

void CStateManagerBurer::setup_substates()
{
    if (current_substate == eStateBurerScanning)
    {
        SStateDataAction data;

        data.action = ACT_LOOK_AROUND;
        data.sound_type = MonsterSound::eMonsterSoundIdle;
        data.sound_delay = object->db().m_dwIdleSndDelay;

        get_state_current()->fill_data_with(&data, sizeof(SStateDataAction));
        return;
    }
}
