#include "StdAfx.h"
#include "poltergeist.h"
#include "poltergeist_state_manager.h"

#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"

#include "poltergeist_state_rest.h"
#include "ai/monsters/states/monster_state_eat.h"
#include "ai/monsters/states/monster_state_attack.h"
#include "ai/monsters/states/monster_state_panic.h"
#include "poltergeist_state_attack_hidden.h"
#include "ai/monsters/states/monster_state_hear_int_sound.h"
#include "ai/monsters/states/monster_state_hear_danger_sound.h"
#include "ai/monsters/states/monster_state_hitted.h"
#include "EntityCondition.h"

CStateManagerPoltergeist::CStateManagerPoltergeist(CPoltergeist* obj) : inherited(obj)
{
    add_state(eStateRest, new CPoltergeistStateRest<CPoltergeist>(obj));
    add_state(eStateEat, new CStateMonsterEat<CPoltergeist>(obj));
    add_state(eStateAttack_AttackHidden, new CStatePoltergeistAttackHidden<CPoltergeist>(obj));
    add_state(eStatePanic, new CStateMonsterPanic<CPoltergeist>(obj));
    add_state(eStateHitted, new CStateMonsterHitted<CPoltergeist>(obj));
    add_state(eStateHearInterestingSound, new CStateMonsterHearInterestingSound<CPoltergeist>(obj));
    add_state(eStateHearDangerousSound, new CStateMonsterHearDangerousSound<CPoltergeist>(obj));
}

CStateManagerPoltergeist::~CStateManagerPoltergeist() {}
void CStateManagerPoltergeist::reinit()
{
    inherited::reinit();

    time_next_flame_attack = 0;
    time_next_tele_attack = 0;
    time_next_scare_attack = 0;
}

void CStateManagerPoltergeist::execute()
{
    u32 state_id = u32(-1);

    if (object->EnemyMan.get_enemy() && object->detected_enemy())
    {
        state_id = eStateAttack_AttackHidden;
    }
    else
    {
        state_id = eStateRest;
    }

    // const CEntityAlive* enemy	= object->EnemyMan.get_enemy();

    // if (enemy) {
    //	if (object->is_hidden()) state_id = eStateAttack_AttackHidden;
    //	else {
    //		switch (object->EnemyMan.get_danger_type()) {
    //			case eStrong:	state_id = eStatePanic; break;
    //			case eWeak:		state_id = eStateAttack; break;
    //		}
    //	}
    //} else if (object->HitMemory.is_hit() && !object->is_hidden()) {
    //	state_id = eStateHitted;
    //} else if (object->hear_dangerous_sound) {
    //	if (!object->is_hidden()) state_id = eStateHearDangerousSound;
    //	else state_id = eStateHearInterestingSound;
    //} else if (object->hear_interesting_sound ) {
    //	state_id = eStateHearInterestingSound;
    //} else {
    //	if (can_eat()) state_id = eStateEat;
    //	else state_id = eStateRest;
    //
    //	if (state_id == eStateEat) {
    //		if (object->CorpseMan.get_corpse()->Position().distance_to(object->Position()) < 5.f) {
    //			if (object->is_hidden()) {
    //				object->CEnergyHolder::deactivate();
    //			}
    //
    //			object->DisableHide();
    //		}
    //	}

    //}

    ////if (state_id == eStateAttack_AttackHidden) polter_attack();

    // if ((prev_substate == eStateEat) && (state_id != eStateEat))
    //	object->EnableHide();

    select_state(state_id);

    // выполнить текущее состояние
    get_state_current()->execute();

    prev_substate = current_substate;
}

#define TIME_SEEN_FOR_FIRE 5000

void CStateManagerPoltergeist::polter_attack()
{
    // u32 cur_time = Device.dwTimeGlobal;
    // const CEntityAlive* enemy	= object->EnemyMan.get_enemy();
    //
    // bool b_aggressive = object->conditions().GetHealth() < 0.5f;

    // if ((time_next_flame_attack < cur_time) && (object->EnemyMan.get_enemy_time_last_seen() + TIME_SEEN_FOR_FIRE >
    // cur_time)) {
    //

    //	object->FireFlame(enemy);
    //	time_next_flame_attack = cur_time + Random.randI(object->m_flame_delay.min, (b_aggressive) ?
    // object->m_flame_delay.aggressive : object->m_flame_delay.normal);
    //}

    // if (time_next_tele_attack < cur_time) {
    //	//object->ProcessTelekinesis(enemy);
    //	time_next_tele_attack = cur_time + Random.randI(object->m_tele_delay.min, (b_aggressive) ?
    // object->m_tele_delay.aggressive : object->m_tele_delay.normal);
    //}

    // if (time_next_scare_attack < cur_time) {
    //	if (Random.randI(2))
    //		object->PhysicalImpulse(enemy->Position());
    //	else
    //		object->StrangeSounds(enemy->Position());
    //
    //	time_next_scare_attack = cur_time + Random.randI(object->m_scare_delay.min, (b_aggressive) ?
    // object->m_scare_delay.aggressive : object->m_scare_delay.normal);
    //}
}
