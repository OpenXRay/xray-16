#include "StdAfx.h"
#include "control_run_attack.h"
#include "basemonster/base_monster.h"
#include "monster_velocity_space.h"
#include "control_animation_base.h"
#include "control_direction_base.h"
#include "control_movement_base.h"

void CControlRunAttack::load(LPCSTR section)
{
    read_distance(section, "Run_Attack_Dist", m_min_dist, m_max_dist);
    read_delay(section, "Run_Attack_Delay", m_min_delay, m_max_delay);
}

void CControlRunAttack::reinit()
{
    CControl_ComCustom<>::reinit();

    m_time_next_attack = 0;
}

void CControlRunAttack::activate()
{
    m_man->capture_pure(this);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);
    m_man->subscribe(this, ControlCom::eventAnimationStart);

    m_man->path_stop(this);
    m_man->move_stop(this);

    //////////////////////////////////////////////////////////////////////////

    SControlDirectionData* ctrl_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_dir);
    ctrl_dir->heading.target_speed = 3.f;
    ctrl_dir->heading.target_angle = m_man->direction().angle_to_target(m_object->EnemyMan.get_enemy()->Position());

    //////////////////////////////////////////////////////////////////////////

    SControlAnimationData* ctrl_anim = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_anim);

    ctrl_anim->global.set_motion(
        smart_cast<IKinematicsAnimated*>(m_object->Visual())->ID_Cycle_Safe("stand_attack_run_0"));
    ctrl_anim->global.actual = false;
}

void CControlRunAttack::on_release()
{
    m_man->unlock(this, ControlCom::eControlPath);
    m_man->release_pure(this);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);
    m_man->unsubscribe(this, ControlCom::eventAnimationStart);
}

bool CControlRunAttack::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;

    const CEntityAlive* enemy = m_object->EnemyMan.get_enemy();
    if (!enemy)
        return false;
    // check if faced enemy
    if (!m_man->direction().is_face_target(enemy, PI_DIV_6))
        return false;

    float dist = enemy->Position().distance_to(m_object->Position());
    // check distance to enemy
    if ((dist > m_max_dist) || (dist < m_min_dist))
        return false;

    // check if run state, speed
    SVelocityParam& velocity_run = m_object->move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    if (!fsimilar(m_man->movement().velocity_current(), velocity_run.velocity.linear, 2.f))
        return false;

    if (m_time_next_attack > time())
        return false;

    return true;
}

void CControlRunAttack::on_event(ControlCom::EEventType type, ControlCom::IEventData* dat)
{
    switch (type)
    {
    case ControlCom::eventAnimationEnd:
        m_time_next_attack = time() + Random.randI(m_min_delay, m_max_delay);
        m_man->notify(ControlCom::eventRunAttackEnd, 0);
        break;
    case ControlCom::eventAnimationStart: // handle blend params
    {
        // set animation speed
        SControlAnimationData* ctrl_data_anim =
            (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
        VERIFY(ctrl_data_anim);

        CBlend* blend = m_man->animation().current_blend();
        VERIFY(blend);

        // animation time
        float anim_time = blend->timeTotal / blend->speed;

        // run velocity
        u32 velocity_mask = MonsterMovement::eVelocityParameterRunNormal;
        SVelocityParam& velocity = m_object->move().get_velocity(velocity_mask);

        // distance
        float path_dist = anim_time * velocity.velocity.linear;

        Fvector dir;
        dir.sub(m_object->EnemyMan.get_enemy()->Position(), m_object->Position());
        dir.normalize_safe();

        Fvector target_position;
        target_position.mad(m_object->Position(), dir, path_dist);

        if (!m_man->build_path_line(
                this, target_position, u32(-1), velocity_mask | MonsterMovement::eVelocityParameterStand))
        {
            m_man->notify(ControlCom::eventRunAttackEnd, 0);
        }
        else
        {
            // enable path
            SControlPathBuilderData* ctrl_path = (SControlPathBuilderData*)m_man->data(this, ControlCom::eControlPath);
            VERIFY(ctrl_path);
            ctrl_path->enable = true;

            m_man->lock(this, ControlCom::eControlPath);

            SControlMovementData* ctrl_move = (SControlMovementData*)m_man->data(this, ControlCom::eControlMovement);
            VERIFY(ctrl_move);
            ctrl_move->velocity_target = velocity.velocity.linear;
            ctrl_move->acc = flt_max;
        }
    }
    break;
    }
}
