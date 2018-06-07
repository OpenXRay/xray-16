#pragma once

#include "sound_player.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateZombieAttackRunAbstract CStateZombieAttackRun<_Object>

TEMPLATE_SPECIALIZATION
CStateZombieAttackRunAbstract::CStateZombieAttackRun(_Object* obj)
    : inherited(obj), m_time_action_change(0), action() {}

TEMPLATE_SPECIALIZATION
CStateZombieAttackRunAbstract::~CStateZombieAttackRun() {}
TEMPLATE_SPECIALIZATION
void CStateZombieAttackRunAbstract::initialize()
{
    inherited::initialize();
    m_time_action_change = 0;
    action = ACT_WALK_FWD;

    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateZombieAttackRunAbstract::execute()
{
    const float dist = this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position());

    this->object->path().set_try_min_time(false);

    // установка параметров функциональных блоков
    this->object->path().set_target_point(this->object->EnemyMan.get_enemy_position(), this->object->EnemyMan.get_enemy_vertex());
    this->object->path().set_rebuild_time(100 + u32(50.f * dist));
    this->object->path().set_distance_to_end(2.5f);
    this->object->path().set_use_covers(false);

    //////////////////////////////////////////////////////////////////////////
    // обработать squad-данные
    //////////////////////////////////////////////////////////////////////////
    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    bool squad_active = squad && squad->SquadActive();

    // Получить команду
    SSquadCommand command;
    squad->GetCommand(this->object, command);
    if (!squad_active || (command.type != SC_ATTACK))
        squad_active = false;
    //////////////////////////////////////////////////////////////////////////

    if (squad_active)
    {
        this->object->path().set_use_dest_orient(true);
        this->object->path().set_dest_direction(command.direction);
    }
    else
        this->object->path().set_use_dest_orient(false);

    choose_action();
    this->object->anim().m_tAction = action;

    if (action == ACT_RUN)
        this->object->path().set_try_min_time(true);

    this->object->sound().play(MonsterSound::eMonsterSoundAggressive, 0, 0, this->object->db().m_dwAttackSndDelay);
    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);
}

TEMPLATE_SPECIALIZATION
bool CStateZombieAttackRunAbstract::check_completion()
{
    const float m_fDistMin = this->object->MeleeChecker.get_min_distance();
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());

    if (dist < m_fDistMin)
        return true;

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateZombieAttackRunAbstract::check_start_conditions()
{
    const float m_fDistMax = this->object->MeleeChecker.get_max_distance();
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());

    if (dist > m_fDistMax)
        return true;

    return false;
}

#define CHANGE_ACTION_FROM_RUN 10000

TEMPLATE_SPECIALIZATION
void CStateZombieAttackRunAbstract::choose_action()
{
    // for test
    action = this->object->HitMemory.is_hit() ? ACT_RUN : ACT_WALK_FWD;

    //// check if its a strong monster
    // if (object->Rank() > 50) {
    //	action = object->HitMemory.is_hit() ?  ACT_RUN : ACT_WALK_FWD;
    //	return;
    //}
    //
    // if ((action == ACT_RUN) && (m_time_action_change + CHANGE_ACTION_FROM_RUN > time())) return;

    //// установка параметров функциональных блоков
    // if (object->HitMemory.is_hit() && (object->conditions().GetHealth() < 0.5f))
    //	action = ACT_RUN;
    // else
    //	action = ACT_WALK_FWD;

    // m_time_action_change = time();
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateZombieAttackRunAbstract
