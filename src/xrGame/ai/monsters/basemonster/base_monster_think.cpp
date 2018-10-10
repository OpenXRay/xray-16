#include "StdAfx.h"
#include "base_monster.h"
#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "xrEngine/profiler.h"
#include "ai/monsters/state_manager.h"
#include "xrPhysics/PhysicsShell.h"
#include "detail_path_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "Level.h"
#include "ai/monsters/control_animation_base.h"

void CBaseMonster::Think()
{
    START_PROFILE("Base Monster/Think");

    if (!g_Alive() || getDestroy())
        return;

    // Инициализировать
    InitThink();
    anim().ScheduledInit();

    // Обновить память
    START_PROFILE("Base Monster/Think/Update Memory");
    UpdateMemory();
    STOP_PROFILE;

    // Обновить сквад
    START_PROFILE("Base Monster/Think/Update Squad");
    monster_squad().update(this);
    STOP_PROFILE;

    // Запустить FSM
    START_PROFILE("Base Monster/Think/FSM");
    update_fsm();
    STOP_PROFILE;

    STOP_PROFILE;
}

void CBaseMonster::update_fsm()
{
    StateMan->update();

    // завершить обработку установленных в FSM параметров
    post_fsm_update();

    TranslateActionToPathParams();

    // информировать squad о своих целях
    squad_notify();

#ifdef DEBUG
    debug_fsm();
#endif
}

void CBaseMonster::post_fsm_update()
{
    if (!EnemyMan.get_enemy())
        return;

    EMonsterState state = StateMan->get_state_type();

    // Look at enemy while running
    m_bRunTurnLeft = m_bRunTurnRight = false;

    Fvector direction;
    if (is_state(state, eStateAttack) && control().path_builder().is_moving_on_path() &&
        control().path_builder().detail().try_get_direction(direction))
    {
        Fvector const self_to_enemy = Fvector().sub(EnemyMan.get_enemy()->Position(), Position());
        if (magnitude(self_to_enemy) > 3.f)
        {
            float dir_yaw = direction.getH();
            float yaw_target = self_to_enemy.getH();

            float angle_diff = angle_difference(yaw_target, dir_yaw);

            if ((angle_diff > PI_DIV_3) && (angle_diff < 5 * PI_DIV_6))
            {
                if (from_right(dir_yaw, yaw_target))
                    m_bRunTurnRight = true;
                else
                    m_bRunTurnLeft = true;
            }
        }
    }
}

void CBaseMonster::squad_notify()
{
    CMonsterSquad* squad = monster_squad().get_squad(this);
    SMemberGoal goal;

    EMonsterState state = StateMan->get_state_type();

    if (is_state(state, eStateAttack))
    {
        goal.type = MG_AttackEnemy;
        goal.entity = const_cast<CEntityAlive*>(EnemyMan.get_enemy());
    }
    else if (is_state(state, eStateRest))
    {
        goal.entity = squad->GetLeader();

        if (state == eStateRest_Idle)
            goal.type = MG_Rest;
        else if (state == eStateRest_WalkGraphPoint)
            goal.type = MG_WalkGraph;
        else if (state == eStateRest_MoveToHomePoint)
            goal.type = MG_WalkGraph;
        else if (state == eStateCustomMoveToRestrictor)
            goal.type = MG_WalkGraph;
        else if (state == eStateRest_WalkToCover)
            goal.type = MG_WalkGraph;
        else if (state == eStateRest_LookOpenPlace)
            goal.type = MG_Rest;
        else
            goal.entity = 0;
    }
    else if (is_state(state, eStateSquad))
    {
        goal.type = MG_Rest;
        goal.entity = squad->GetLeader();
    }

    squad->UpdateGoal(this, goal);
}
