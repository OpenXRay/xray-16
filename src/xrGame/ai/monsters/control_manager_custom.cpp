#include "StdAfx.h"
#include "control_manager_custom.h"
#include "basemonster/base_monster.h"
#include "control_sequencer.h"
#include "control_run_attack.h"
#include "control_threaten.h"
#include "xrPhysics/PhysicsShell.h"
#include "detail_path_manager.h"
#include "Level.h"
#include "control_animation_base.h"
#include "control_critical_wound.h"

CControlManagerCustom::CControlManagerCustom()
    : m_critical_wound(nullptr), m_threaten_anim(nullptr), m_threaten_time(0)
{
    m_sequencer = 0;
    m_triple_anim = 0;
    m_rotation_jump = 0;
    m_jump = 0;
    m_run_attack = 0;
    m_threaten = 0;
    m_melee_jump = 0;
}

CControlManagerCustom::~CControlManagerCustom()
{
    xr_delete(m_sequencer);
    xr_delete(m_triple_anim);
    xr_delete(m_rotation_jump);
    xr_delete(m_jump);
    xr_delete(m_run_attack);
    xr_delete(m_threaten);
    xr_delete(m_melee_jump);
    xr_delete(m_critical_wound);
}

void CControlManagerCustom::reinit()
{
    inherited::reinit();
    m_rot_jump_data.clear();
}

void CControlManagerCustom::add_ability(ControlCom::EControlType type)
{
    switch (type)
    {
    case ControlCom::eControlSequencer:
        m_sequencer = new CAnimationSequencer();
        m_man->add(m_sequencer, ControlCom::eControlSequencer);
        break;
    case ControlCom::eControlTripleAnimation:
        m_triple_anim = new CAnimationTriple();
        m_man->add(m_triple_anim, ControlCom::eControlTripleAnimation);
        break;
    case ControlCom::eControlRotationJump:
        m_rotation_jump = new CControlRotationJump();
        m_man->add(m_rotation_jump, ControlCom::eControlRotationJump);
        break;
    case ControlCom::eControlJump:
        m_jump = new CControlJump();
        m_man->add(m_jump, ControlCom::eControlJump);
        break;
    case ControlCom::eControlRunAttack:
        m_run_attack = new CControlRunAttack();
        m_man->add(m_run_attack, ControlCom::eControlRunAttack);
        break;
    case ControlCom::eControlThreaten:
        m_threaten = new CControlThreaten();
        m_man->add(m_threaten, ControlCom::eControlThreaten);
        set_threaten_data(0, 0.f);
        break;
    case ControlCom::eControlMeleeJump:
        m_melee_jump = new CControlMeleeJump();
        m_man->add(m_melee_jump, ControlCom::eControlMeleeJump);
        break;
    case ControlCom::eComCriticalWound:
        m_critical_wound = new CControlCriticalWound();
        m_man->add(m_critical_wound, ControlCom::eComCriticalWound);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

void CControlManagerCustom::on_start_control(ControlCom::EControlType type)
{
    switch (type)
    {
    case ControlCom::eControlSequencer: m_man->subscribe(this, ControlCom::eventSequenceEnd); break;
    case ControlCom::eControlTripleAnimation: m_man->subscribe(this, ControlCom::eventTAChange); break;
    case ControlCom::eControlJump: m_man->subscribe(this, ControlCom::eventJumpEnd); break;
    case ControlCom::eControlRotationJump: m_man->subscribe(this, ControlCom::eventRotationJumpEnd); break;
    case ControlCom::eControlMeleeJump: m_man->subscribe(this, ControlCom::eventMeleeJumpEnd); break;
    case ControlCom::eControlRunAttack: m_man->subscribe(this, ControlCom::eventRunAttackEnd); break;
    case ControlCom::eControlThreaten: m_man->subscribe(this, ControlCom::eventThreatenEnd); break;
    case ControlCom::eComCriticalWound: m_man->subscribe(this, ControlCom::eventCriticalWoundEnd); break;
    }
}

void CControlManagerCustom::on_stop_control(ControlCom::EControlType type)
{
    switch (type)
    {
    case ControlCom::eControlSequencer: m_man->unsubscribe(this, ControlCom::eventSequenceEnd); break;
    case ControlCom::eControlTripleAnimation: m_man->unsubscribe(this, ControlCom::eventTAChange); break;
    case ControlCom::eControlJump: m_man->unsubscribe(this, ControlCom::eventJumpEnd); break;
    case ControlCom::eControlRotationJump: m_man->unsubscribe(this, ControlCom::eventRotationJumpEnd); break;
    case ControlCom::eControlMeleeJump: m_man->unsubscribe(this, ControlCom::eventMeleeJumpEnd); break;
    case ControlCom::eControlRunAttack: m_man->unsubscribe(this, ControlCom::eventRunAttackEnd); break;
    case ControlCom::eControlThreaten: m_man->unsubscribe(this, ControlCom::eventThreatenEnd); break;
    case ControlCom::eComCriticalWound: m_man->unsubscribe(this, ControlCom::eventCriticalWoundEnd); break;
    }
}

void CControlManagerCustom::on_event(ControlCom::EEventType type, ControlCom::IEventData* data)
{
    switch (type)
    {
    case ControlCom::eventSequenceEnd: m_man->release(this, ControlCom::eControlSequencer); break;
    case ControlCom::eventTAChange:
    {
        STripleAnimEventData* event_data = (STripleAnimEventData*)data;
        if (event_data->m_current_state == eStateNone)
            m_man->release(this, ControlCom::eControlTripleAnimation);

        break;
    }
    case ControlCom::eventJumpEnd: m_man->release(this, ControlCom::eControlJump); break;
    case ControlCom::eventRotationJumpEnd: m_man->release(this, ControlCom::eControlRotationJump); break;
    case ControlCom::eventMeleeJumpEnd: m_man->release(this, ControlCom::eControlMeleeJump); break;
    case ControlCom::eventRunAttackEnd: m_man->release(this, ControlCom::eControlRunAttack); break;
    case ControlCom::eventThreatenEnd: m_man->release(this, ControlCom::eControlThreaten); break;
    case ControlCom::eventCriticalWoundEnd: m_man->release(this, ControlCom::eComCriticalWound); break;
    }
}

void CControlManagerCustom::update_frame() {}
void CControlManagerCustom::update_schedule()
{
    if (m_threaten)
        check_threaten();
    if (m_jump)
    {
        check_attack_jump();
        // check_jump_over_physics	();
    }
    if (m_rotation_jump)
        check_rotation_jump();
    if (m_run_attack)
        check_run_attack();
    if (m_melee_jump)
        check_melee_jump();
}

//////////////////////////////////////////////////////////////////////////
void CControlManagerCustom::ta_fill_data(
    SAnimationTripleData& data, LPCSTR s1, LPCSTR s2, LPCSTR s3, bool execute_once, bool skip_prep, u32 capture_type)
{
    // Load triple animations
    IKinematicsAnimated* skel_animated = smart_cast<IKinematicsAnimated*>(m_object->Visual());
    data.pool[0] = skel_animated->ID_Cycle_Safe(s1);
    VERIFY(data.pool[0]);
    data.pool[1] = skel_animated->ID_Cycle_Safe(s2);
    VERIFY(data.pool[1]);
    data.pool[2] = skel_animated->ID_Cycle_Safe(s3);
    VERIFY(data.pool[2]);
    data.execute_once = execute_once;
    data.skip_prepare = skip_prep;
    data.capture_type = capture_type;
}

void CControlManagerCustom::ta_activate(const SAnimationTripleData& data)
{
    if (!m_man->check_start_conditions(ControlCom::eControlTripleAnimation))
        return;

    m_man->capture(this, ControlCom::eControlTripleAnimation);

    SAnimationTripleData* ctrl_data = (SAnimationTripleData*)m_man->data(this, ControlCom::eControlTripleAnimation);
    VERIFY(ctrl_data);

    ctrl_data->pool[0] = data.pool[0];
    ctrl_data->pool[1] = data.pool[1];
    ctrl_data->pool[2] = data.pool[2];
    ctrl_data->skip_prepare = data.skip_prepare;
    ctrl_data->execute_once = data.execute_once;
    ctrl_data->capture_type = data.capture_type;

    m_man->activate(ControlCom::eControlTripleAnimation);
}

void CControlManagerCustom::ta_pointbreak()
{
    if (ta_is_active())
        m_triple_anim->pointbreak();
}

bool CControlManagerCustom::ta_is_active() { return (m_triple_anim->is_active()); }
bool CControlManagerCustom::ta_is_active(const SAnimationTripleData& data)
{
    if (!m_triple_anim->is_active())
        return false;

    SAnimationTripleData* ctrl_data = (SAnimationTripleData*)m_man->data(this, ControlCom::eControlTripleAnimation);
    VERIFY(ctrl_data);

    return ((ctrl_data->pool[0] == data.pool[0]) && (ctrl_data->pool[1] == data.pool[1]) &&
        (ctrl_data->pool[2] == data.pool[2]));
}

void CControlManagerCustom::ta_deactivate() { m_man->release(this, ControlCom::eControlTripleAnimation); }
///////////////////////////////////////////////////////////////////////////////////////////////////
// Работа с последовательностями
void CControlManagerCustom::seq_init()
{
    m_man->capture(this, ControlCom::eControlSequencer);

    SAnimationSequencerData* ctrl_data = (SAnimationSequencerData*)m_man->data(this, ControlCom::eControlSequencer);
    if (!ctrl_data)
        return;

    ctrl_data->motions.clear();
}

void CControlManagerCustom::seq_add(MotionID motion)
{
    SAnimationSequencerData* ctrl_data = (SAnimationSequencerData*)m_man->data(this, ControlCom::eControlSequencer);
    if (!ctrl_data)
        return;

    ctrl_data->motions.push_back(motion);
}

void CControlManagerCustom::seq_switch() { m_man->activate(ControlCom::eControlSequencer); }
void CControlManagerCustom::seq_run(MotionID motion)
{
    if (!m_man->check_start_conditions(ControlCom::eControlSequencer))
        return;

    m_man->capture(this, ControlCom::eControlSequencer);

    SAnimationSequencerData* ctrl_data = (SAnimationSequencerData*)m_man->data(this, ControlCom::eControlSequencer);
    if (!ctrl_data)
        return;

    ctrl_data->motions.clear();
    ctrl_data->motions.push_back(motion);

    m_man->activate(ControlCom::eControlSequencer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Jumping
///////////////////////////////////////////////////////////////////////////////////////////////////
void CControlManagerCustom::jump(IGameObject* obj, const SControlJumpData& ta)
{
    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return;

    if (m_object->GetScriptControl())
        return;

    m_man->capture(this, ControlCom::eControlJump);

    SControlJumpData* ctrl_data = (SControlJumpData*)m_man->data(this, ControlCom::eControlJump);
    VERIFY(ctrl_data);

    ctrl_data->target_object = obj;
    ctrl_data->target_position = obj->Position();
    ctrl_data->flags = ta.flags;
    ctrl_data->state_prepare.motion = ta.state_prepare.motion;
    ctrl_data->state_prepare_in_move.motion = ta.state_prepare_in_move.motion;
    ctrl_data->state_prepare_in_move.velocity_mask = ta.state_prepare_in_move.velocity_mask;
    ctrl_data->state_glide.motion = ta.state_glide.motion;
    ctrl_data->state_ground.motion = ta.state_ground.motion;
    ctrl_data->state_ground.velocity_mask = ta.state_ground.velocity_mask;
    ctrl_data->force_factor = -1.f;

    m_man->activate(ControlCom::eControlJump);
}

void CControlManagerCustom::load_jump_data(
    LPCSTR s1, LPCSTR s2, LPCSTR s3, LPCSTR s4, u32 vel_mask_prepare, u32 vel_mask_ground, u32 flags)
{
    IKinematicsAnimated* skel_animated = smart_cast<IKinematicsAnimated*>(m_object->Visual());
    if (!skel_animated)
    {
        return; // monster is dead, so no skeleton (early return due to bug: 18755)
    }

    m_jump->setup_data().flags.assign(flags);

    if (s1)
    {
        m_jump->setup_data().state_prepare.motion = skel_animated->ID_Cycle_Safe(s1);
        VERIFY(m_jump->setup_data().state_prepare.motion);
    }
    else
        m_jump->setup_data().state_prepare.motion.invalidate();

    if (s2)
    {
        m_jump->setup_data().state_prepare_in_move.motion = skel_animated->ID_Cycle_Safe(s2);
        VERIFY(m_jump->setup_data().state_prepare_in_move.motion);
        m_jump->setup_data().flags._or (SControlJumpData::ePrepareInMove);
    }
    else
        m_jump->setup_data().state_prepare_in_move.motion.invalidate();

    m_jump->setup_data().state_glide.motion = skel_animated->ID_Cycle_Safe(s3);
    VERIFY(m_jump->setup_data().state_glide.motion);

    if (s4)
    {
        m_jump->setup_data().state_ground.motion = skel_animated->ID_Cycle_Safe(s4);
        VERIFY(m_jump->setup_data().state_ground.motion);
    }
    else
    {
        m_jump->setup_data().state_ground.motion.invalidate();
        m_jump->setup_data().flags._or (SControlJumpData::eGroundSkip);
    }

    if (!s1 && !s2)
    {
        m_jump->setup_data().flags._or (SControlJumpData::ePrepareSkip);
    }

    m_jump->setup_data().flags._or (SControlJumpData::eGlidePlayAnimOnce);
    m_jump->setup_data().flags._or (SControlJumpData::eGlideOnPrepareFailed);

    m_jump->setup_data().state_prepare_in_move.velocity_mask = vel_mask_prepare;
    m_jump->setup_data().state_ground.velocity_mask = vel_mask_ground;

    m_jump->setup_data().force_factor = -1.f;
}

bool CControlManagerCustom::is_jumping() { return m_jump && m_jump->is_active(); }
bool CControlManagerCustom::jump(const SControlJumpData& ta)
{
    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return false;

    if (m_object->GetScriptControl())
        return false;

    m_man->capture(this, ControlCom::eControlJump);

    SControlJumpData* ctrl_data = (SControlJumpData*)m_man->data(this, ControlCom::eControlJump);
    VERIFY(ctrl_data);

    ctrl_data->target_object = ta.target_object;
    ctrl_data->target_position = ta.target_position;
    ctrl_data->flags = ta.flags;
    ctrl_data->state_prepare.motion = ta.state_prepare.motion;
    ctrl_data->state_prepare_in_move.motion = ta.state_prepare_in_move.motion;
    ctrl_data->state_prepare_in_move.velocity_mask = ta.state_prepare_in_move.velocity_mask;
    ctrl_data->state_glide.motion = ta.state_glide.motion;
    ctrl_data->state_ground.motion = ta.state_ground.motion;
    ctrl_data->state_ground.velocity_mask = ta.state_ground.velocity_mask;
    ctrl_data->force_factor = -1.f;

    m_man->activate(ControlCom::eControlJump);
    return true;
}

void CControlManagerCustom::jump(const Fvector& position)
{
    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return;

    m_man->capture(this, ControlCom::eControlJump);

    SControlJumpData* ctrl_data = (SControlJumpData*)m_man->data(this, ControlCom::eControlJump);
    VERIFY(ctrl_data);

    ctrl_data->target_object = 0;
    ctrl_data->target_position = position;
    ctrl_data->flags._or (SControlJumpData::ePrepareSkip);
    ctrl_data->force_factor = -1.f;

    m_man->activate(ControlCom::eControlJump);
}
void CControlManagerCustom::script_capture(ControlCom::EControlType type)
{
    if (!m_man->check_start_conditions(type))
        return;
    m_man->capture(this, type);
}

void CControlManagerCustom::script_release(ControlCom::EControlType type)
{
    if (m_man->check_capturer(this, type))
        m_man->release(this, type);
}

void CControlManagerCustom::script_jump(const Fvector& position, float factor)
{
    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return;

    m_man->capture(this, ControlCom::eControlJump);

    SControlJumpData* ctrl_data = (SControlJumpData*)m_man->data(this, ControlCom::eControlJump);
    VERIFY(ctrl_data);

    ctrl_data->target_object = 0;
    ctrl_data->target_position = position;
    ctrl_data->force_factor = factor;

    m_man->activate(ControlCom::eControlJump);
}

//////////////////////////////////////////////////////////////////////////
// Services
//////////////////////////////////////////////////////////////////////////
void CControlManagerCustom::check_attack_jump()
{
    if (!m_object->EnemyMan.get_enemy())
        return;
    if (m_object->GetScriptControl())
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlJump))
        return;
    if (!m_object->EnemyMan.see_enemy_now())
        return;

    CEntityAlive* target = const_cast<CEntityAlive*>(m_object->EnemyMan.get_enemy());
    if (!m_jump->can_jump(target))
        return;

    if (m_man->check_start_conditions(ControlCom::eControlJump))
    {
        m_jump->setup_data().flags.set(SControlJumpData::ePrepareSkip, false);
        m_jump->setup_data().flags.set(SControlJumpData::eUseTargetPosition, false);
        m_jump->setup_data().flags.set(SControlJumpData::eUseAutoAim, true);
        m_jump->setup_data().target_object = target;
        m_jump->setup_data().target_position = target->Position();

        jump(m_jump->setup_data());
    }
}

bool CControlManagerCustom::check_if_jump_possible(Fvector const& target, bool const full_check)
{
    if (!m_object->check_start_conditions(ControlCom::eControlJump))
        return false;
    if (full_check && !m_jump->can_jump(target, false))
        return false;

    return m_man->check_start_conditions(ControlCom::eControlJump);
}

bool CControlManagerCustom::jump_if_possible(Fvector const& target, CEntityAlive* const target_object,
    bool const use_direction_to_target, bool const use_velocity_bounce, bool const check_possibility)
{
    if (!m_object->check_start_conditions(ControlCom::eControlJump))
        return false;

    bool const aggressive_jump = target_object ? m_object->can_use_agressive_jump(target_object) : NULL;
    if (check_possibility && !m_jump->can_jump(target, aggressive_jump))
        return false;

    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return false;

    m_jump->setup_data().flags.set(SControlJumpData::eUseAutoAim, use_direction_to_target);
    m_jump->setup_data().flags.set(SControlJumpData::eUseTargetPosition, true);
    m_jump->setup_data().flags.set(SControlJumpData::eDontUseVelocityBounce, !use_velocity_bounce);
    m_jump->setup_data().flags.set(SControlJumpData::ePrepareSkip, true);
    m_jump->setup_data().target_object = target_object;
    m_jump->setup_data().target_position = target;

    return jump(m_jump->setup_data());
}

#define MAX_DIST_SUM 6.f

void CControlManagerCustom::check_jump_over_physics()
{
    if (!m_man->path_builder().is_moving_on_path())
        return;
    if (!m_man->check_start_conditions(ControlCom::eControlJump))
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlJump))
        return;
    if (m_object->GetScriptControl())
        return;

    Fvector prev_pos = m_object->Position();
    float dist_sum = 0.f;

    for (u32 i = m_man->path_builder().detail().curr_travel_point_index();
         i < m_man->path_builder().detail().path().size(); i++)
    {
        const DetailPathManager::STravelPathPoint& travel_point = m_man->path_builder().detail().path()[i];

        // получить список объектов вокруг врага
        m_nearest.clear();
        Level().ObjectSpace.GetNearest(m_nearest, travel_point.position, m_object->Radius(), NULL);

        for (u32 k = 0; k < m_nearest.size(); k++)
        {
            CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(m_nearest[k]);
            if (!obj || !obj->PPhysicsShell() || !obj->PPhysicsShell()->isActive() || (obj->Radius() < 0.5f))
                continue;
            if (m_object->Position().distance_to(obj->Position()) < MAX_DIST_SUM / 2)
                continue;

            Fvector dir = Fvector().sub(travel_point.position, m_object->Position());

            // проверка на  Field-Of-View
            float my_h = m_object->Direction().getH();
            float h = dir.getH();

            float from = angle_normalize(my_h - deg(8));
            float to = angle_normalize(my_h + deg(8));

            if (!is_angle_between(h, from, to))
                continue;

            dir = Fvector().sub(obj->Position(), m_object->Position());

            // вычислить целевую позицию для прыжка
            Fvector target;
            obj->Center(target);
            target.y += obj->Radius();
            // --------------------------------------------------------

            m_jump->setup_data().flags.set(SControlJumpData::ePrepareSkip, true);
            m_jump->setup_data().target_object = 0;
            m_jump->setup_data().target_position = target;

            jump(m_jump->setup_data());

            return;
        }

        dist_sum += prev_pos.distance_to(travel_point.position);
        if (dist_sum > MAX_DIST_SUM)
            break;

        prev_pos = travel_point.position;
    }
}

//////////////////////////////////////////////////////////////////////////
// Rotation Jump
//////////////////////////////////////////////////////////////////////////

void CControlManagerCustom::check_rotation_jump()
{
    if (!m_man->check_start_conditions(ControlCom::eControlRotationJump))
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlRotationJump))
        return;

    VERIFY(!m_rot_jump_data.empty());

    m_man->capture(this, ControlCom::eControlRotationJump);

    SControlRotationJumpData* ctrl_data =
        (SControlRotationJumpData*)m_man->data(this, ControlCom::eControlRotationJump);
    VERIFY(ctrl_data);

    (*ctrl_data) = m_rot_jump_data[Random.randI(m_rot_jump_data.size())];

    m_man->activate(ControlCom::eControlRotationJump);
}

void CControlManagerCustom::add_rotation_jump_data(
    LPCSTR left1, LPCSTR left2, LPCSTR right1, LPCSTR right2, float angle, u32 flags)
{
    SControlRotationJumpData data;
    fill_rotation_data(data, left1, left2, right1, right2, angle, flags);

    m_rot_jump_data.push_back(data);
}
//////////////////////////////////////////////////////////////////////////

void CControlManagerCustom::check_run_attack()
{
    if (!m_man->check_start_conditions(ControlCom::eControlRunAttack))
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlRunAttack))
        return;

    m_man->capture(this, ControlCom::eControlRunAttack);
    m_man->activate(ControlCom::eControlRunAttack);
}

void CControlManagerCustom::check_threaten()
{
    if (!m_man->check_start_conditions(ControlCom::eControlThreaten))
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlThreaten))
        return;

    m_man->capture(this, ControlCom::eControlThreaten);

    SControlThreatenData* ctrl_data = (SControlThreatenData*)m_man->data(this, ControlCom::eControlThreaten);
    VERIFY(ctrl_data);
    ctrl_data->animation = m_threaten_anim;
    ctrl_data->time = m_threaten_time;

    m_man->activate(ControlCom::eControlThreaten);
}

//////////////////////////////////////////////////////////////////////////
// MELEE JUMP
//////////////////////////////////////////////////////////////////////////

void CControlManagerCustom::add_melee_jump_data(LPCSTR left, LPCSTR right)
{
    IKinematicsAnimated* skel_animated = smart_cast<IKinematicsAnimated*>(m_object->Visual());
    m_melee_jump_data.anim_ls = skel_animated->ID_Cycle_Safe(left);
    m_melee_jump_data.anim_rs = skel_animated->ID_Cycle_Safe(right);
}

void CControlManagerCustom::check_melee_jump()
{
    if (!m_man->check_start_conditions(ControlCom::eControlMeleeJump))
        return;
    if (!m_object->check_start_conditions(ControlCom::eControlMeleeJump))
        return;

    m_man->capture(this, ControlCom::eControlMeleeJump);

    SControlMeleeJumpData* ctrl_data = (SControlMeleeJumpData*)m_man->data(this, ControlCom::eControlMeleeJump);
    VERIFY(ctrl_data);

    (*ctrl_data) = m_melee_jump_data;

    m_man->activate(ControlCom::eControlMeleeJump);
}

//////////////////////////////////////////////////////////////////////////
// Fill Rotation Data
//////////////////////////////////////////////////////////////////////////
void CControlManagerCustom::fill_rotation_data(
    SControlRotationJumpData& data, LPCSTR left1, LPCSTR left2, LPCSTR right1, LPCSTR right2, float angle, u32 flags)
{
    VERIFY(m_object->Visual());
    IKinematicsAnimated* skeleton_animated = smart_cast<IKinematicsAnimated*>(m_object->Visual());

    data.flags.assign(flags);
    data.turn_angle = angle;

    MotionID motion;
    if (left1)
    {
        motion = skeleton_animated->ID_Cycle_Safe(left1);
        data.anim_stop_ls = motion;
        m_object->anim().AddAnimTranslation(motion, left1);
    }
    else
    {
        data.anim_stop_ls.invalidate();
    }

    if (left2)
    {
        motion = skeleton_animated->ID_Cycle_Safe(left2);
        data.anim_run_ls = motion;
        m_object->anim().AddAnimTranslation(motion, left2);
    }
    else
    {
        data.anim_run_ls.invalidate();
    }

    if (right1)
    {
        motion = skeleton_animated->ID_Cycle_Safe(right1);
        data.anim_stop_rs = motion;
        m_object->anim().AddAnimTranslation(motion, right1);
    }
    else
    {
        data.anim_stop_rs.invalidate();
    }

    if (right2)
    {
        motion = skeleton_animated->ID_Cycle_Safe(right2);
        data.anim_run_rs = motion;
        m_object->anim().AddAnimTranslation(motion, right2);
    }
    else
    {
        data.anim_run_rs.invalidate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CControlManagerCustom::critical_wound(LPCSTR anim)
{
    if (!m_man->check_start_conditions(ControlCom::eComCriticalWound))
        return;

    m_man->capture(this, ControlCom::eComCriticalWound);

    SControlCriticalWoundData* ctrl_data = (SControlCriticalWoundData*)m_man->data(this, ControlCom::eComCriticalWound);
    if (!ctrl_data)
        return;

    ctrl_data->animation = anim;

    m_man->activate(ControlCom::eComCriticalWound);
}
//////////////////////////////////////////////////////////////////////////

void CControlManagerCustom::remove_links(IGameObject* object)
{
    if (m_jump)
        m_jump->remove_links(object);
}
