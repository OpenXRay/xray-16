#include "StdAfx.h"
#include "chimera.h"
#include "chimera_state_manager.h"

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
#include "chimera_state_threaten.h"
#include "ai/monsters/states/state_test_state.h"

#include "chimera_attack_state.h"

CStateManagerChimera::CStateManagerChimera(CChimera* obj) : inherited(obj)
{
    add_state(eStateRest, new CStateMonsterRest<CChimera>(obj));
    add_state(eStatePanic, new CStateMonsterPanic<CChimera>(obj));
    add_state(eStateAttack, new ChimeraAttackState<CChimera>(obj));
    add_state(eStateEat, new CStateMonsterEat<CChimera>(obj));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CChimera>(obj));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CChimera>(obj));
    // 	add_state(eStateHitted,					new CStateMonsterHitted<CChimera>(obj));
    // 	add_state(eStateThreaten,				new CStateChimeraThreaten<CChimera>(obj));
    // 	add_state(eStateCustom,					new CStateMonsterTestState<CChimera>(obj));
}

CStateManagerChimera::~CStateManagerChimera() {}
void CStateManagerChimera::execute()
{
    u32 state_id = u32(-1);

    const CEntityAlive* enemy = object->EnemyMan.get_enemy();

    if (enemy)
    {
        // if (check_state(eStateThreaten)) state_id = eStateThreaten;
        switch (object->EnemyMan.get_danger_type())
        {
        case eStrong: state_id = eStatePanic; break;
        case eWeak: state_id = eStateAttack; break;
        }
        // 	else if (object->HitMemory.is_hit()) {
        // 		state_id = eStateHitted;
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

    // state_id = eStateCustom;

    select_state(state_id);

    // выполнить текущее состояние
    get_state_current()->execute();

    prev_substate = current_substate;
}
