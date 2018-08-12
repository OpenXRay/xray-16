#include "StdAfx.h"
#include "poltergeist.h"
#include "poltergeist_state_manager.h"
#include "CharacterPhysicsSupport.h"
#include "PHMovementControl.h"
#include "PhysicsShellHolder.h"
#include "ai_debug.h"
#include "poltergeist_movement.h"
#include "detail_path_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "Level.h"
#include "level_debug.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"
#include "xrPhysics/PhysicsShell.h"
#include "Actor.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"
#include "ActorEffector.h"
#include "ActorCondition.h"

void SetActorVisibility(u16 who, float value);

CPoltergeist::CPoltergeist()
{
    StateMan = new CStateManagerPoltergeist(this);

    invisible_vel.set(0.1f, 0.1f);

    m_flame = 0;
    m_tele = 0;
    m_actor_ignore = false;
}

CPoltergeist::~CPoltergeist()
{
    remove_pp_effector();

    xr_delete(StateMan);
    xr_delete(m_flame);
    xr_delete(m_tele);
}

void CPoltergeist::Load(LPCSTR section)
{
    inherited::Load(section);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);

    invisible_vel.set(pSettings->r_float(section, "Velocity_Invisible_Linear"),
        pSettings->r_float(section, "Velocity_Invisible_Angular"));
    movement().detail().add_velocity(MonsterMovement::eVelocityParameterInvisible,
        CDetailPathManager::STravelParams(invisible_vel.linear, invisible_vel.angular));

    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    SVelocityParam& velocity_run_dmg = move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    // SVelocityParam &velocity_steal		= move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    // SVelocityParam &velocity_drag		= move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRun, "stand_run_fwd_", -1, &velocity_run, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(
        eAnimDie, "stand_idle_", 0, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimMiscAction_00, "fall_down_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimMiscAction_01, "fly_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(
        eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLookAround, "stand_look_around_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_dmg_", -1, &velocity_walk_dmg, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRunDamaged, "stand_walk_dmg_", -1, &velocity_run_dmg, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");

    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimStandIdle);
    anim().LinkAction(ACT_REST, eAnimStandIdle);
    anim().LinkAction(ACT_DRAG, eAnimStandIdle);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimWalkFwd);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimLookAround);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    // READ_IF_EXISTS(pSettings,r_u32,section,"PsyAura_Fake_Delay", 8000);
    // READ_IF_EXISTS(pSettings,r_float,section,"PsyAura_Fake_MaxAddDist", 90.f);

    m_height_change_velocity = READ_IF_EXISTS(pSettings, r_float, section, "Height_Change_Velocity", 0.5f);
    m_height_change_min_time = READ_IF_EXISTS(pSettings, r_u32, section, "Height_Change_Min_Time", 3000);
    m_height_change_max_time = READ_IF_EXISTS(pSettings, r_u32, section, "Height_Change_Max_Time", 10000);
    m_height_min = READ_IF_EXISTS(pSettings, r_float, section, "Height_Min", 0.4f);
    m_height_max = READ_IF_EXISTS(pSettings, r_float, section, "Height_Max", 2.f);

    m_fly_around_level = READ_IF_EXISTS(pSettings, r_float, section, "detection_fly_around_level", 5.f);
    m_fly_around_distance = READ_IF_EXISTS(pSettings, r_float, section, "detection_fly_around_distance", 15.f);

    m_fly_around_change_direction_time =
        READ_IF_EXISTS(pSettings, r_float, section, "detection_fly_around_change_direction_time", 7);

    LPCSTR polter_type = pSettings->r_string(section, "type");

    if (xr_strcmp(polter_type, "flamer") == 0)
    {
        m_flame = new CPolterFlame(this);
        m_flame->load(section);
    }
    else
    {
        m_tele = new CPolterTele(this);
        m_tele->load(section);
    }

    m_detection_pp_effector_name = READ_IF_EXISTS(pSettings, r_string, section, "detection_pp_effector_name", "");
    m_detection_near_range_factor = READ_IF_EXISTS(pSettings, r_float, section, "detection_near_range_factor", 2.f);
    m_detection_far_range_factor = READ_IF_EXISTS(pSettings, r_float, section, "detection_far_range_factor", 1.f);
    m_detection_speed_factor = READ_IF_EXISTS(pSettings, r_float, section, "detection_speed_factor", 1.f);
    m_detection_loose_speed = READ_IF_EXISTS(pSettings, r_float, section, "detection_loose_speed", 5.f);
    m_detection_far_range = READ_IF_EXISTS(pSettings, r_float, section, "detection_far_range", 20.f);
    m_detection_success_level = READ_IF_EXISTS(pSettings, r_float, section, "detection_success_level", 4.f);
    m_detection_max_level = READ_IF_EXISTS(pSettings, r_float, section, "detection_max_level", 100.f);

    m_current_detection_level = 0;
    m_last_detection_time = 0;
    m_detection_pp_type_index = 0;

    PostLoad(section);
}

float CPoltergeist::get_post_process_factor() const
{
    float factor = m_current_detection_level / m_detection_success_level;
    clamp(factor, 0.f, 1.f);
    return factor;
}

bool CPoltergeist::check_work_condition() const { return g_Alive() && Actor() && Actor()->g_Alive(); }
void CPoltergeist::remove_pp_effector()
{
    if (m_detection_pp_type_index != 0 && Actor())
    {
        RemoveEffector(Actor(), m_detection_pp_type_index);
        m_detection_pp_type_index = 0;
    }
}

void CPoltergeist::update_detection()
{
    if (!check_work_condition())
    {
        remove_pp_effector();
        return;
    }

    //-------------------------------------------------------------------
    // calculate detection level and turn on/off post processing effect
    //-------------------------------------------------------------------
    Fvector const actor_pos = Actor()->Position();
    float const dist2actor = actor_pos.distance_to(Position());

    float const time_passed_sec = float(Device.dwTimeGlobal - m_last_detection_time) / 1000.f;
    m_last_detection_time = Device.dwTimeGlobal;

    if (!get_actor_ignore() && time_passed_sec > 0.f && time_passed_sec < 2.f && dist2actor < get_detection_far_range())
    {
        float const relative_range = dist2actor / get_detection_far_range();
        float const range_factor = relative_range * get_detection_far_range_factor() +
            (1.f - relative_range) * get_detection_near_range_factor();

        float const speed_factor = get_detection_speed_factor();
        float const raw_speed = m_last_actor_pos.distance_to(actor_pos) / time_passed_sec;

        float const speed = powf(1.f + raw_speed, speed_factor) - 1.f;

        float const actor_psy_immunity = Actor()->conditions().GetHitImmunity(ALife::eHitTypeTelepatic);
        m_current_detection_level += time_passed_sec * 0.03f * actor_psy_immunity * range_factor * speed;
    }

    m_current_detection_level -= time_passed_sec * get_detection_loose_speed();
    m_current_detection_level = (m_current_detection_level < 0) ? 0.f : m_current_detection_level;

    if (m_current_detection_level > m_detection_max_level)
        m_current_detection_level = m_detection_max_level;

    if (time_passed_sec != 0.f)
        m_last_actor_pos = actor_pos;

    SetActorVisibility(ID(), get_post_process_factor());

    if (m_current_detection_level > 0.01f && m_detection_pp_effector_name && m_detection_pp_effector_name[0])
    {
        if (!m_detection_pp_type_index)
        {
            for (m_detection_pp_type_index = (u32)effPoltergeistTeleDetectStartEffect;
                 Actor()->Cameras().GetPPEffector((EEffectorPPType)m_detection_pp_type_index);
                 ++m_detection_pp_type_index)
            {
                ;
            }

            AddEffector(Actor(), m_detection_pp_type_index, m_detection_pp_effector_name,
                GET_KOEFF_FUNC(this, &CPoltergeist::get_post_process_factor));
        }
    }
    else if (m_detection_pp_type_index != 0)
    {
        RemoveEffector(Actor(), m_detection_pp_type_index);
        m_detection_pp_type_index = 0;
    }
}

bool CPoltergeist::detected_enemy() { return get_current_detection_level() > m_fly_around_level; }
void CPoltergeist::reload(LPCSTR section)
{
    inherited::reload(section);
    Energy::reload(section, "Invisible_");
}

void CPoltergeist::reinit()
{
    inherited::reinit();
    Energy::reinit();

    m_current_position = Position();

    target_height = 0.3f;
    time_height_updated = 0;

    Energy::set_auto_activate();
    Energy::set_auto_deactivate();
    Energy::enable();
    m_actor_ignore = false;

    // start hidden
    state_invisible = true;
    setVisible(false);
    m_current_position = Position();
    character_physics_support()->movement()->DestroyCharacter();

    m_height = 0.3f;
    time_height_updated = 0;
    m_actor_ignore = false;

    DisableHide();
}

void CPoltergeist::Hide()
{
    if (state_invisible)
        return;

    state_invisible = true;
    setVisible(false);

    m_current_position = Position();
    character_physics_support()->movement()->DestroyCharacter();

    ability()->on_hide();
}

void CPoltergeist::Show()
{
    if (!state_invisible)
        return;

    state_invisible = false;

    setVisible(TRUE);

    com_man().seq_run(anim().get_motion_id(eAnimMiscAction_00));

    Position() = m_current_position;
    character_physics_support()->movement()->SetPosition(Position());
    character_physics_support()->movement()->CreateCharacter();

    ability()->on_show();
}

void CPoltergeist::renderable_Render()
{
    Visual()->getVisData().hom_frame = Device.dwFrame;
    inherited::renderable_Render();
}

void CPoltergeist::UpdateCL()
{
    update_detection();
    inherited::UpdateCL();

    def_lerp(m_height, target_height, m_height_change_velocity, client_update_fdelta());

    ability()->update_frame();

    if (Actor()->memory().visual().visible_now(this) && Actor()->Position().distance_to(Position()) < 85.f)
    {
        MakeMeCrow();
    }

    //	Visual()->getVisData().hom_frame = Device.dwFrame;
}

void CPoltergeist::ForceFinalAnimation()
{
    if (state_invisible)
        anim().SetCurAnim(eAnimMiscAction_01);
}

void CPoltergeist::shedule_Update(u32 dt)
{
    if (!check_work_condition())
    {
        remove_pp_effector();
    }

    inherited::shedule_Update(dt);
    CTelekinesis::schedule_update();
    Energy::schedule_update();

    UpdateHeight();

    ability()->update_schedule();
}

BOOL CPoltergeist::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);
    VERIFY(character_physics_support());
    VERIFY(character_physics_support()->movement());
    character_physics_support()->movement()->DestroyCharacter();
    // спаунится нивидимым
    setVisible(false);
    ability()->on_hide();

    return (TRUE);
}

void CPoltergeist::net_Destroy()
{
    inherited::net_Destroy();
    Energy::disable();

    ability()->on_destroy();
}

void CPoltergeist::Die(IGameObject* who)
{
    // 	if (m_tele) {
    // 		if (state_invisible) {
    // 			setVisible(true);
    //
    // 			if (PPhysicsShell()) {
    // 				Fmatrix M;
    // 				M.set							(XFORM());
    // 				M.translate_over				(m_current_position);
    // 				PPhysicsShell()->SetTransform	(M);
    // 			} else
    // 				Position() = m_current_position;
    // 		}
    // 	}

    inherited::Die(who);
    Energy::disable();

    ability()->on_die();
}

void CPoltergeist::Hit(SHit* pHDS)
{
    ability()->on_hit(pHDS);

    if (pHDS->who == Actor())
    {
        m_current_detection_level = m_detection_max_level;
    }

    inherited::Hit(pHDS);
}

void CPoltergeist::UpdateHeight()
{
    if (!state_invisible)
        return;

    u32 cur_time = Device.dwTimeGlobal;

    if (time_height_updated < cur_time)
    {
        time_height_updated = cur_time + Random.randI(m_height_change_min_time, m_height_change_max_time);
        target_height = Random.randF(m_height_min, m_height_max);
    }
}

void CPoltergeist::on_activate()
{
    if (m_disable_hide)
        return;

    Hide();

    m_height = 0.3f;
    time_height_updated = 0;
}

void CPoltergeist::on_deactivate()
{
    if (m_disable_hide)
        return;

    Show();
}

CMovementManager* CPoltergeist::create_movement_manager()
{
    m_movement_manager = new CPoltergeisMovementManager(this);

    control().add(m_movement_manager, ControlCom::eControlPath);
    control().install_path_manager(m_movement_manager);
    control().set_base_controller(m_path_base, ControlCom::eControlPath);

    return (m_movement_manager);
}

void CPoltergeist::net_Relcase(IGameObject* O)
{
    inherited::net_Relcase(O);
    CTelekinesis::remove_links(O);
}

float CPoltergeist::get_detection_near_range_factor()
{
    return override_if_debug("detection_near_range_factor", m_detection_near_range_factor);
}

float CPoltergeist::get_detection_far_range_factor()
{
    return override_if_debug("detection_far_range_factor", m_detection_far_range_factor);
}

float CPoltergeist::get_detection_speed_factor()
{
    return override_if_debug("detection_speed_factor", m_detection_speed_factor);
}

float CPoltergeist::get_detection_loose_speed()
{
    return override_if_debug("detection_loose_speed", m_detection_loose_speed);
}

float CPoltergeist::get_detection_far_range()
{
    return override_if_debug("detection_far_range", m_detection_far_range);
}

float CPoltergeist::get_detection_success_level()
{
    return override_if_debug("detection_success_level", m_detection_success_level);
}

#ifdef DEBUG
CBaseMonster::SDebugInfo CPoltergeist::show_debug_info()
{
    CBaseMonster::SDebugInfo info = inherited::show_debug_info();
    if (!info.active)
        return CBaseMonster::SDebugInfo();

    string128 text;
    xr_sprintf(text, "Invisibility Value = [%f]", Energy::get_value());
    DBG().text(this).add_item(text, info.x, info.y += info.delta_y, info.color);
    DBG().text(this).add_item(
        "---------------------------------------", info.x, info.y += info.delta_y, info.delimiter_color);

    return CBaseMonster::SDebugInfo();
}
#endif
