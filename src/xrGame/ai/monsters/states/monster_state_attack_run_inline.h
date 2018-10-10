#pragma once

#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/ai_monster_squad_manager.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterAttackRunAbstract CStateMonsterAttackRun<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::initialize()
{
    inherited::initialize();
    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::execute()
{
    // установка параметров функциональных блоков
    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);

    u32 const level_vertex = this->object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
    Fvector const level_pos = ai().level_graph().vertex_position(level_vertex);
    this->object->path().set_target_point(level_pos, level_vertex);

    if (level_vertex == this->object->ai_location().level_vertex_id())
        this->object->set_action(ACT_STAND_IDLE);
    else
        this->object->set_action(ACT_RUN);

    this->object->path().set_rebuild_time(this->object->get_attack_rebuild_time());
    this->object->path().set_use_covers();
    this->object->path().set_cover_params(0.1f, 30.f, 1.f, 30.f);
    this->object->path().set_try_min_time(false);
    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
    this->object->path().extrapolate_path(true);

    // обработать squad инфо
    this->object->path().set_use_dest_orient(false);

    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    if (squad && squad->SquadActive())
    {
        // Получить команду
        SSquadCommand command;
        squad->GetCommand(this->object, command);

        if (command.type == SC_ATTACK)
        {
            this->object->path().set_use_dest_orient(true);
            this->object->path().set_dest_direction(command.direction);
        }
    }
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::finalize()
{
    inherited::finalize();
    this->object->path().extrapolate_path(false);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackRunAbstract::critical_finalize()
{
    inherited::critical_finalize();
    this->object->path().extrapolate_path(false);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAbstract::check_completion()
{
    const float m_fDistMin = this->object->MeleeChecker.get_min_distance();
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());

    if (dist < m_fDistMin)
        return true;

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackRunAbstract::check_start_conditions()
{
    const float m_fDistMax = this->object->MeleeChecker.get_max_distance();
    const float dist = this->object->MeleeChecker.distance_to_enemy(this->object->EnemyMan.get_enemy());

    if (dist > m_fDistMax)
        return true;

    return false;
}
