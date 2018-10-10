#include "StdAfx.h"
#include "anti_aim_ability.h"
#include "basemonster/base_monster.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "xrCore/_vector3d_ext.h"
#include "control_animation_base.h"
#include "Inventory.h"
#include "Weapon.h"

namespace detail
{
static pcstr const s_anti_aim_timeout_string = "anti_aim_timeout";
static pcstr const s_anti_aim_effectors_string = "anti_aim_effectors";
static pcstr const s_anti_aim_freeze_time_string = "anti_aim_freeze_time";
static pcstr const s_anti_aim_max_angle_string = "anti_aim_max_angle";
static pcstr const s_anti_aim_detection_gain_speed_string = "anti_aim_detection_gain_speed";
static pcstr const s_anti_aim_detection_loose_speed_string = "anti_aim_detection_loose_speed";

} // namespace detail

anti_aim_ability::anti_aim_ability(CBaseMonster* const object) : m_object(object)
{
    m_effector_id = 0;
    m_max_angle = 0.5f;
    m_last_activated_tick = 0;
    m_last_detection_tick = 0;
    m_last_angle = M_PI;
    m_callback.clear();
}

anti_aim_ability::~anti_aim_ability() { do_deactivate(); }
void anti_aim_ability::load_from_ini(CInifile const* ini, pcstr const section)
{
    using namespace detail;

    m_timeout = READ_IF_EXISTS(ini, r_float, section, s_anti_aim_timeout_string, 5.f);
    m_freeze_time = READ_IF_EXISTS(ini, r_float, section, s_anti_aim_freeze_time_string, 1.f);
    m_max_angle = READ_IF_EXISTS(ini, r_float, section, s_anti_aim_max_angle_string, 0.5f);
    m_detection_gain_speed = READ_IF_EXISTS(ini, r_float, section, s_anti_aim_detection_gain_speed_string, 1.f);
    m_detection_loose_speed = READ_IF_EXISTS(ini, r_float, section, s_anti_aim_detection_loose_speed_string, 0.1f);
    pcstr effectors = READ_IF_EXISTS(ini, r_string, section, s_anti_aim_effectors_string, NULL);

    if (effectors)
    {
        u32 const num_effectors = _GetItemCount(effectors, ',');
        m_effectors.resize(num_effectors);
        for (u32 i = 0; i < num_effectors; ++i)
        {
            char effector_name[1024];
            _GetItem(effectors, i, effector_name, ',');
            m_effectors[i] = effector_name;
        }
    }
}

void anti_aim_ability::on_monster_death() { do_deactivate(); }
bool anti_aim_ability::can_detect()
{
    CEntityAlive const* enemy = m_object->EnemyMan.get_enemy();
    Fvector const self2enemy = enemy->Position() - m_object->Position();
    Fvector const self_dir = m_object->Direction();
    float angle = angle_between_vectors(self2enemy, self_dir);

    return angle < deg2rad(70.f);
}

bool anti_aim_ability::check_start_condition()
{
    if (is_active())
    {
        return false;
    }

    if (m_object->GetScriptControl() && !m_object->get_force_anti_aim())
    {
        return false;
    }

    if (!m_object->check_start_conditions(ControlCom::eAntiAim))
    {
        return false;
    }

    if (m_man->is_captured(ControlCom::eControlAnimation))
    {
        return false;
    }

    if (m_man->is_captured(ControlCom::eControlPath))
    {
        return false;
    }

    if (m_man->is_captured(ControlCom::eControlMovement))
    {
        return false;
    }

    if (is_active())
    {
        return false;
    }

    if (!check_update_condition())
    {
        return false;
    }

    if (m_object->anim().has_override_animation())
    {
        return false;
    }

    if (m_detection_level < 1.f && !m_object->get_force_anti_aim())
    {
        return false;
    }

    if (!can_detect())
    {
        return false;
    }

    return true;
}

void anti_aim_ability::activate()
{
    m_man->capture(this, ControlCom::eControlAnimation);
    m_man->capture(this, ControlCom::eControlPath);
    m_man->capture(this, ControlCom::eControlMovement);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);

    m_man->path_stop(this);
    m_man->move_stop(this);

    m_last_activated_tick = Device.dwTimeGlobal;

    m_animation_hit_tick =
        Device.dwTimeGlobal + (TTime)(m_object->anim().get_animation_hit_time(eAnimAntiAimAbility, 0) * 1000);

    MotionID motion;
    float anim_length;
    m_object->anim().get_animation_info(eAnimAntiAimAbility, 0, motion, anim_length);

    SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_data);

    ctrl_data->global.set_motion(motion);
    ctrl_data->global.actual = false;

    m_animation_end_tick = Device.dwTimeGlobal + (TTime)(anim_length * 1000);

    m_object->set_force_anti_aim(false);
}

void anti_aim_ability::start_camera_effector()
{
    VERIFY(!m_effector_id);
    VERIFY(m_effectors.size());
    pcstr const effector_name = m_effectors[rand() % m_effectors.size()].c_str();

    m_effector_id = Actor()->Cameras().RequestCamEffectorId();

    CAnimatorCamEffector* cam_eff = new CAnimatorCamEffector();
    cam_eff->SetType((ECamEffectorType)m_effector_id);
    cam_eff->SetCyclic(false);

    if (pSettings->line_exist(effector_name, "cam_eff_hud_affect"))
    {
        cam_eff->SetHudAffect(!!pSettings->r_bool(effector_name, "cam_eff_hud_affect"));
    }

    LPCSTR fn = pSettings->r_string(effector_name, "cam_eff_name");
    cam_eff->Start(fn);

    m_camera_effector_end_tick = Device.dwTimeGlobal + (TTime)(cam_eff->GetAnimatorLength() * 1000);
    m_camera_effector_end_tick = _max(m_camera_effector_end_tick, m_animation_end_tick);

    Actor()->Cameras().AddCamEffector(cam_eff);

    if (m_callback)
    {
        m_callback();
    }
}

void anti_aim_ability::do_deactivate()
{
    if (is_active())
    {
        if (m_object->Visual()) // when m_object is destroying, visual might be dead already
        {
            m_man->deactivate(this);
        }

        m_object->set_force_anti_aim(false);
    }
}

void anti_aim_ability::deactivate()
{
    if (is_active())
    {
        return;
    }

    m_man->release(this, ControlCom::eControlAnimation);
    m_man->release(this, ControlCom::eControlPath);
    m_man->release(this, ControlCom::eControlMovement);

    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

    if (Actor())
    {
        Actor()->Cameras().RemoveCamEffector((ECamEffectorType)m_effector_id);
    }

    m_effector_id = 0;
    m_last_detection_tick = Device.dwTimeGlobal;
    m_last_angle = M_PI;
    m_detection_level = 0.f;
}

float anti_aim_ability::calculate_angle() const
{
    float const opposite_angle_return_value = M_PI;

    if (!m_object->EnemyMan.see_enemy_now(Actor()))
    {
        return opposite_angle_return_value;
    }

    Fvector const self_dir = Actor()->Cameras().Direction();

    Fvector monster_center;
    m_object->Center(monster_center);
    Fvector monster_head = get_head_position(m_object);

    Fvector const actor_head = get_head_position(Actor());
    Fvector const to_monster_center = monster_center - actor_head;
    Fvector const to_monster_head = monster_head - actor_head;
    float const max_deviation = angle_between_vectors(to_monster_center, to_monster_head);
    float const deviation = angle_between_vectors(to_monster_center, self_dir);

    return _max(0.f, deviation - max_deviation);
}

#include "level_debug.h"
#include "debug_text_tree.h"

extern CActor* g_actor;

bool anti_aim_ability::check_update_condition() const
{
    if (!m_object->g_Alive() || !g_actor || !Actor()->g_Alive())
        return false;

    CEntityAlive const* enemy = m_object->EnemyMan.get_enemy();
    if (enemy != Actor())
        return false;

    if (!smart_cast<CWeapon*>(Actor()->inventory().ActiveItem()))
        return false;

    return true;
}

void anti_aim_ability::update_schedule()
{
#ifdef DEBUG_STATE
    DBG().get_text_tree().clear();
    debug::text_tree& text_tree = DBG().get_text_tree().find_or_add("ActorView");
    text_tree.add_line("detection_level", m_detection_level);
#endif // #ifdef DEBUG_STATE

    if (!check_update_condition())
    {
        do_deactivate();
        return;
    }

    if (check_start_condition())
    {
        m_man->activate(ControlCom::eAntiAim);
    }

    if (is_active())
    {
#ifdef DEBUG_STATE
        text_tree.add_line("state", "activated");
#endif // #ifdef DEBUG_STATE

        if (Device.dwTimeGlobal < m_animation_hit_tick)
        {
            return;
        }
        if (m_effector_id == 0)
        {
            start_camera_effector();
        }
        if (Device.dwTimeGlobal < m_camera_effector_end_tick)
        {
            return;
        }
        do_deactivate();
    }

    if (Device.dwTimeGlobal < m_last_activated_tick + (TTime)(m_timeout * 1000))
    {
#ifdef DEBUG_STATE
        text_tree.add_line("state", "colddown");
#endif // #ifdef DEBUG_STATE
        return;
    }

#ifdef DEBUG_STATE
    text_tree.add_line("state", "deactivated");
#endif // #ifdef DEBUG_STATE

    if (m_last_detection_tick == 0)
    {
        m_last_detection_tick = Device.dwTimeGlobal;
    }

    float const detect_delta = (Device.dwTimeGlobal - m_last_detection_tick) / 1000.f;
    m_last_detection_tick = Device.dwTimeGlobal;

    float const angle = calculate_angle();
    float const average_angle = std::min(m_max_angle, (angle + m_last_angle) / 2);
    float const relative_angle = (m_max_angle - average_angle) / m_max_angle;
    float const detect_speed = can_detect() ? _sqr(relative_angle) * m_detection_gain_speed : 0;

    m_detection_level += (detect_speed - m_detection_loose_speed) * detect_delta;
    clamp(m_detection_level, 0.f, 1.f);
    m_last_angle = angle;
}
