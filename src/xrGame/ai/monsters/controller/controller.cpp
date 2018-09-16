#include "stdafx.h"
#include "controller.h"
#include "controller_state_manager.h"
#include "ai/Monsters/controlled_entity.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "ActorCondition.h"
#include "ai/Monsters/ai_monster_effector.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Level.h"
#include "sound_player.h"
#include "ai_monster_space.h"
#include "UIGameCustom.h"
#include "ui/UIStatic.h"
#include "ai/Monsters/monster_velocity_space.h"
#include "level_debug.h"
#include "game_object_space.h"
#include "detail_path_manager.h"
#include "ai_space.h"
#include "cover_point.h"
#include "cover_manager.h"
#include "ai/Monsters/Controller/controller_animation.h"
#include "ai/Monsters/Controller/controller_direction.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_path_builder_base.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Inventory.h"
#include "monster_community.h"
#include "character_community.h"
#include "InventoryOwner.h"
#include "xrServerEntities/character_info.h"
#include "ai/Monsters/Controller/controller_psy_hit.h"
#include "ai/Monsters/monster_cover_manager.h"

const u32 _pmt_psy_attack_delay = 2000;
const float _pmt_psy_attack_min_angle = deg(5);

namespace detail
{
namespace controller
{
// default settings for tube fire:
const u32 default_tube_condition_see_duration = 50;
const u32 default_tube_condition_min_delay = 10000;
const float default_tube_condition_min_distance = 10;
const float default_stamina_hit = 0.2f;

} // namespace controller
} // namespace detail

CController::CController()
{
    StateMan = new CStateManagerController(this);
    time_control_hit_started = 0;

    m_psy_hit = new CControllerPsyHit();

    control().add(m_psy_hit, ControlCom::eComCustom1);

#ifdef _DEBUG
    P1.set(0.f, 0.f, 0.f);
    P2.set(0.f, 0.f, 0.f);
#endif
}

CController::~CController()
{
    xr_delete(StateMan);
    xr_delete(m_psy_hit);
}

void CController::Load(LPCSTR section)
{
    inherited::Load(section);

    // Load Control FX texture
    //	m_UIControlFX.Init(pSettings->r_string(section, "control_fx_texture"), "hud\\default",0,0,0);
    //	m_UIControlFX2.Init(pSettings->r_string(section, "control_fx_texture2"), "hud\\default",0,0,0);

    m_max_controlled_number = pSettings->r_u8(section, "Max_Controlled_Count");
    m_controlled_objects.reserve(m_max_controlled_number);

    anim().accel_load(section);
    // anim().accel_chain_add		(eAnimWalkFwd,		eAnimRun);
    // anim().accel_chain_add		(eAnimWalkDamaged,	eAnimRunDamaged);

    GEnv.Sound->create(
        control_start_sound, pSettings->r_string(section, "sound_control_start"), st_Effect, SOUND_TYPE_WORLD);
    GEnv.Sound->create(control_hit_sound, pSettings->r_string(section, "sound_control_hit"), st_Effect, SOUND_TYPE_WORLD);

    anim().AddReplacedAnim(&m_bDamaged, eAnimStandIdle, eAnimStandDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);

    // Load control postprocess --------------------------------------------------------
    LPCSTR ppi_section = pSettings->r_string(section, "control_effector");
    m_control_effector.ppi.duality.h = pSettings->r_float(ppi_section, "duality_h");
    m_control_effector.ppi.duality.v = pSettings->r_float(ppi_section, "duality_v");
    m_control_effector.ppi.gray = pSettings->r_float(ppi_section, "gray");
    m_control_effector.ppi.blur = pSettings->r_float(ppi_section, "blur");
    m_control_effector.ppi.noise.intensity = pSettings->r_float(ppi_section, "noise_intensity");
    m_control_effector.ppi.noise.grain = pSettings->r_float(ppi_section, "noise_grain");
    m_control_effector.ppi.noise.fps = pSettings->r_float(ppi_section, "noise_fps");
    VERIFY(!fis_zero(m_control_effector.ppi.noise.fps));

    sscanf(pSettings->r_string(ppi_section, "color_base"), "%f,%f,%f", &m_control_effector.ppi.color_base.r,
        &m_control_effector.ppi.color_base.g, &m_control_effector.ppi.color_base.b);
    sscanf(pSettings->r_string(ppi_section, "color_gray"), "%f,%f,%f", &m_control_effector.ppi.color_gray.r,
        &m_control_effector.ppi.color_gray.g, &m_control_effector.ppi.color_gray.b);
    sscanf(pSettings->r_string(ppi_section, "color_add"), "%f,%f,%f", &m_control_effector.ppi.color_add.r,
        &m_control_effector.ppi.color_add.g, &m_control_effector.ppi.color_add.b);

    m_control_effector.time = pSettings->r_float(ppi_section, "time");
    m_control_effector.time_attack = pSettings->r_float(ppi_section, "time_attack");
    m_control_effector.time_release = pSettings->r_float(ppi_section, "time_release");

    m_control_effector.ce_time = pSettings->r_float(ppi_section, "ce_time");
    m_control_effector.ce_amplitude = pSettings->r_float(ppi_section, "ce_amplitude");
    m_control_effector.ce_period_number = pSettings->r_float(ppi_section, "ce_period_number");
    m_control_effector.ce_power = pSettings->r_float(ppi_section, "ce_power");

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    // SVelocityParam &velocity_run		= move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    // SVelocityParam &velocity_walk_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    // SVelocityParam &velocity_run_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    // SVelocityParam &velocity_drag		= move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandDamaged, "stand_idle_dmg_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSitIdle, "sit_idle_", -1, &velocity_none, PS_SIT);
    anim().AddAnim(eAnimEat, "sit_eat_", -1, &velocity_none, PS_SIT);
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimRun, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimRunDamaged, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND);
    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimDie, "stand_die_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimStandSitDown, "stand_sit_down_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSitStandUp, "sit_stand_up_", -1, &velocity_none, PS_SIT);
    anim().AddAnim(eAnimSleep, "sit_sleep_", -1, &velocity_none, PS_SIT);

    // anim().AddAnim(eAnimStandIdle,		"new_torso_steal_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimStandTurnLeft,	"new_torso_steal_",			-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimStandTurnRight,	"new_torso_steal_",			-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimStandDamaged,	"new_torso_steal_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimSitIdle,		"sit_idle_",			-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimEat,			"sit_eat_",				-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimWalkFwd,		"new_torso_steal_",			-1, &velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimWalkDamaged,	"new_torso_steal_",			-1, &velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimRun,			"new_torso_steal_",			-1,	&velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimRunDamaged,		"new_torso_steal_",			-1, &velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimAttack,			"stand_attack_",		-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimSteal,			"new_torso_steal_",			-1, &velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimCheckCorpse,	"stand_check_corpse_",	-1,	&velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimDie,			"stand_die_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimStandSitDown,	"stand_sit_down_",		-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimSitStandUp,		"sit_stand_up_",		-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimSleep,			"sit_sleep_",			-1, &velocity_none,		PS_SIT);

    // anim().AddAnim(eAnimStandIdle,		"stand_idle_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimStandTurnLeft,	"stand_turn_ls_",		-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimStandTurnRight,	"stand_turn_rs_",		-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimStandDamaged,	"stand_idle_dmg_",		-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimSitIdle,		"sit_idle_",			-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimEat,			"sit_eat_",				-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimWalkFwd,		"stand_walk_fwd_",		-1, &velocity_walk,		PS_STAND);
    // anim().AddAnim(eAnimWalkDamaged,	"stand_walk_dmg_",		-1, &velocity_walk_dmg,	PS_STAND);
    // anim().AddAnim(eAnimRun,			"run_scared_",			-1,	&velocity_run,		PS_STAND);
    // anim().AddAnim(eAnimRunDamaged,		"stand_run_dmg_",		-1, &velocity_run_dmg,	PS_STAND);
    // anim().AddAnim(eAnimAttack,			"stand_attack_",		-1, &velocity_turn,		PS_STAND);
    // anim().AddAnim(eAnimSteal,			"stand_steal_",			-1, &velocity_steal,	PS_STAND);
    // anim().AddAnim(eAnimCheckCorpse,	"stand_check_corpse_",	-1,	&velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimDie,			"stand_die_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimStandSitDown,	"stand_sit_down_",		-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimSitStandUp,		"sit_stand_up_",		-1, &velocity_none,		PS_SIT);
    // anim().AddAnim(eAnimSleep,			"sit_sleep_",			-1, &velocity_none,		PS_SIT);

    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimSitIdle);
    anim().LinkAction(ACT_DRAG, eAnimStandIdle);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimStandIdle);

    anim().AddTransition(PS_STAND, PS_SIT, eAnimStandSitDown, false);
    anim().AddTransition(PS_SIT, PS_STAND, eAnimSitStandUp, false);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    m_velocity_move_fwd.Load(section, "Velocity_MoveFwd");
    m_velocity_move_bkwd.Load(section, "Velocity_MoveBkwd");

    load_friend_community_overrides(section);

    // load
    m_sound_hit_fx.create("affects\\tinnitus3a", st_Effect, sg_SourceType);

    m_sound_aura_left_channel.create("monsters\\controller\\controller_psy_aura_l", st_Effect, sg_SourceType);
    m_sound_aura_right_channel.create("monsters\\controller\\controller_psy_aura_r", st_Effect, sg_SourceType);
    m_sound_aura_hit_left_channel.create("monsters\\controller\\controller_psy_hit_l", st_Effect, sg_SourceType);
    m_sound_aura_hit_right_channel.create("monsters\\controller\\controller_psy_hit_l", st_Effect, sg_SourceType);

    m_sound_tube_start.create("monsters\\controller\\controller_first_hit", st_Effect, sg_SourceType);
    m_sound_tube_pull.create("monsters\\controller\\controller_whoosh", st_Effect, sg_SourceType);
    m_sound_tube_hit_left.create("monsters\\controller\\controller_final_hit_l", st_Effect, sg_SourceType);
    m_sound_tube_hit_right.create("monsters\\controller\\controller_final_hit_r", st_Effect, sg_SourceType);

    m_sound_tube_prepare.create("monsters\\controller\\controller_tube_prepare", st_Effect, sg_SourceType);

    particles_fire = pSettings->r_string(section, "Control_Hit");

    m_tube_damage = pSettings->r_float(section, "tube_damage");
    m_tube_at_once = !!pSettings->r_bool(section, "tube_at_once");

    LPCSTR tube_see_duration_line = "tube_condition_see_duration";
    LPCSTR tube_condition_min_delay_line = "tube_condition_min_delay";
    LPCSTR tube_condition_min_distance_line = "tube_condition_min_distance";

    using namespace detail::controller;
    m_tube_condition_see_duration = pSettings->line_exist(section, tube_see_duration_line) ?
        pSettings->r_u32(section, tube_see_duration_line) :
        default_tube_condition_see_duration;

    m_tube_condition_min_delay = pSettings->line_exist(section, tube_condition_min_delay_line) ?
        pSettings->r_u32(section, tube_condition_min_delay_line) :
        default_tube_condition_min_delay;

    m_tube_condition_min_distance = pSettings->line_exist(section, tube_condition_min_distance_line) ?
        pSettings->r_float(section, tube_condition_min_distance_line) :
        default_tube_condition_min_distance;

    m_stamina_hit = READ_IF_EXISTS(pSettings, r_float, section, "stamina_hit", default_stamina_hit);

    PostLoad(section);
}

void CController::load_friend_community_overrides(LPCSTR section)
{
    LPCSTR src = pSettings->r_string(section, "Friend_Community_Overrides");

    // parse src
    int item_count = _GetItemCount(src);
    m_friend_community_overrides.resize(item_count);
    for (int i = 0; i < item_count; i++)
    {
        string128 st;
        _GetItem(src, i, st);
        m_friend_community_overrides[i] = st;
    }
}

bool CController::is_community_friend_overrides(const CEntityAlive* entity_alive) const
{
    const CInventoryOwner* IO = smart_cast<const CInventoryOwner*>(entity_alive);
    if (!IO)
        return false;
    if (const_cast<CEntityAlive*>(entity_alive)->cast_base_monster())
        return false;

    return (std::find(m_friend_community_overrides.begin(), m_friend_community_overrides.end(),
                IO->CharacterInfo().Community().id()) != m_friend_community_overrides.end());
}

BOOL CController::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);

    return (TRUE);
}

void CController::UpdateControlled()
{
    // если есть враг, проверить может ли быть враг взят под контроль
    if (EnemyMan.get_enemy())
    {
        CControlledEntityBase* entity =
            smart_cast<CControlledEntityBase*>(const_cast<CEntityAlive*>(EnemyMan.get_enemy()));
        if (entity)
        {
            if (!entity->is_under_control() && (m_controlled_objects.size() < m_max_controlled_number))
            {
                // взять под контроль
                entity->set_under_control(this);
                entity->set_task_follow(this);
                m_controlled_objects.push_back(const_cast<CEntityAlive*>(EnemyMan.get_enemy()));
            }
        }
    }
}

void CController::set_controlled_task(u32 task)
{
    if (!HasUnderControl())
        return;

    const CEntity* object =
        ((((ETask)task) == eTaskNone) ? 0 : ((((ETask)task) == eTaskFollow) ? this : EnemyMan.get_enemy()));

    for (u32 i = 0; i < m_controlled_objects.size(); i++)
    {
        CControlledEntityBase* entity = smart_cast<CControlledEntityBase*>(m_controlled_objects[i]);
        entity->get_data().m_object = object;
        entity->get_data().m_task = (ETask)task;
    }
}

void CController::CheckSpecParams(u32 spec_params)
{
    if ((spec_params & ASP_CHECK_CORPSE) == ASP_CHECK_CORPSE)
    {
        com_man().seq_run(anim().get_motion_id(eAnimCheckCorpse));
    }
}

void CController::InitThink()
{
    for (u32 i = 0; i < m_controlled_objects.size(); i++)
    {
        CBaseMonster* base = smart_cast<CBaseMonster*>(m_controlled_objects[i]);
        if (!base)
            continue;
        if (base->EnemyMan.get_enemy())
            EnemyMemory.add_enemy(base->EnemyMan.get_enemy(), base->EnemyMan.get_enemy_position(),
                base->EnemyMan.get_enemy_vertex(), base->EnemyMan.get_enemy_time_last_seen());
    }
}

void CController::play_control_sound_start()
{
    Fvector pos = EnemyMan.get_enemy()->Position();
    pos.y += 1.5f;

    if (control_start_sound._feedback())
        control_start_sound.stop();
    control_start_sound.play_at_pos(const_cast<CEntityAlive*>(EnemyMan.get_enemy()), pos);
}

void CController::play_control_sound_hit()
{
    Fvector pos = EnemyMan.get_enemy()->Position();
    pos.y += 1.5f;

    if (control_hit_sound._feedback())
        control_hit_sound.stop();
    control_hit_sound.play_at_pos(const_cast<CEntityAlive*>(EnemyMan.get_enemy()), pos);
}

void CController::reload(LPCSTR section)
{
    inherited::reload(section);
    com_man().ta_fill_data(
        anim_triple_control, "stand_sit_down_attack_0", "control_attack_0", "sit_stand_up_attack_0", true, false);
}

void CController::reinit()
{
    // must be before inherited call because of its use in ControlAnimation com
    m_mental_state = eStateIdle;

    CControlledActor::reinit();
    inherited::reinit();

    m_psy_fire_start_time = 0;
    m_psy_fire_delay = _pmt_psy_attack_delay;

    control().path_builder().detail().add_velocity(MonsterMovement::eControllerVelocityParameterMoveFwd,
        CDetailPathManager::STravelParams(m_velocity_move_fwd.velocity.linear,
            m_velocity_move_fwd.velocity.angular_path, m_velocity_move_fwd.velocity.angular_real));
    control().path_builder().detail().add_velocity(MonsterMovement::eControllerVelocityParameterMoveBkwd,
        CDetailPathManager::STravelParams(m_velocity_move_bkwd.velocity.linear,
            m_velocity_move_bkwd.velocity.angular_path, m_velocity_move_bkwd.velocity.angular_real));

    m_sndShockEffector = 0;
    active_control_fx = false;
}

void CController::control_hit()
{
    Hit_Psy(const_cast<CEntityAlive*>(EnemyMan.get_enemy()), 30.f);

    // start postprocess
    CActor* pA = const_cast<CActor*>(smart_cast<const CActor*>(EnemyMan.get_enemy()));
    if (!pA)
        return;

    Actor()->Cameras().AddCamEffector(new CMonsterEffectorHit(m_control_effector.ce_time,
        m_control_effector.ce_amplitude, m_control_effector.ce_period_number, m_control_effector.ce_power));
    Actor()->Cameras().AddPPEffector(new CMonsterEffector(m_control_effector.ppi, m_control_effector.time,
        m_control_effector.time_attack, m_control_effector.time_release));

    play_control_sound_hit();
    /*
        active_control_fx			= true;
        time_control_hit_started	= Device.dwTimeGlobal;
    */
}

#define TEXTURE_SIZE_PERCENT 2.f

void CController::UpdateCL()
{
    inherited::UpdateCL();

    if (m_sndShockEffector)
    {
        m_sndShockEffector->Update();
        if (!m_sndShockEffector->InWork())
            xr_delete(m_sndShockEffector);
    }

    if (active_control_fx)
    {
        u32 time_to_show = 150;
        float percent = float((Device.dwTimeGlobal - time_control_hit_started)) / float(time_to_show);
        float percent2 = 1 - (percent - TEXTURE_SIZE_PERCENT) / 2;

        if (percent < TEXTURE_SIZE_PERCENT)
        {
            CurrentGameUI()->RemoveCustomStatic("controller_fx2");
            StaticDrawableWrapper* s = CurrentGameUI()->AddCustomStatic("controller_fx", true);

            float x1 = Device.dwWidth / 2 - ((Device.dwWidth / 2) * percent);
            float y1 = Device.dwHeight / 2 - ((Device.dwHeight / 2) * percent);
            float x2 = Device.dwWidth / 2 + ((Device.dwWidth / 2) * percent);
            float y2 = Device.dwHeight / 2 + ((Device.dwHeight / 2) * percent);

            s->wnd()->SetWndRect(Frect().set(x1, y1, x2 - x1, y2 - y1));
        }
        else if (percent2 > 0)
        {
            CurrentGameUI()->RemoveCustomStatic("controller_fx");
            StaticDrawableWrapper* s = CurrentGameUI()->AddCustomStatic("controller_fx2", true);

            float x1 = Device.dwWidth / 2 - ((Device.dwWidth / 2) * percent2);
            float y1 = Device.dwHeight / 2 - ((Device.dwHeight / 2) * percent2);
            float x2 = Device.dwWidth / 2 + ((Device.dwWidth / 2) * percent2);
            float y2 = Device.dwHeight / 2 + ((Device.dwHeight / 2) * percent2);

            s->wnd()->SetWndRect(Frect().set(x1, y1, x2 - x1, y2 - y1));
        }
        else
        {
            active_control_fx = false;
            CurrentGameUI()->RemoveCustomStatic("controller_fx");
            CurrentGameUI()->RemoveCustomStatic("controller_fx2");
        }
    }
}

void CController::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (g_Alive())
    {
        UpdateControlled();
        if (can_tube_fire())
            tube_fire();
    }

    // DEBUG
    test_covers();
}

void CController::Die(IGameObject* who)
{
    inherited::Die(who);
    FreeFromControl();

    m_psy_hit->on_death();
}

void CController::net_Destroy()
{
    inherited::net_Destroy();

    FreeFromControl();
}

void CController::net_Relcase(IGameObject* O) { inherited::net_Relcase(O); }
void CController::FreeFromControl()
{
    for (u32 i = 0; i < m_controlled_objects.size(); i++)
        smart_cast<CControlledEntityBase*>(m_controlled_objects[i])->free_from_control();
    m_controlled_objects.clear();
}

void CController::OnFreedFromControl(const CEntity* entity)
{
    for (u32 i = 0; i < m_controlled_objects.size(); i++)
        if (m_controlled_objects[i] == entity)
        {
            m_controlled_objects[i] = m_controlled_objects.back();
            m_controlled_objects.pop_back();
            return;
        }
}

//////////////////////////////////////////////////////////////////////////

void CController::draw_fire_particles()
{
    if (!EnemyMan.get_enemy())
        return;
    CEntityAlive* enemy = const_cast<CEntityAlive*>(EnemyMan.get_enemy());
    if (!EnemyMan.see_enemy_now())
        return;

    // вычислить позицию и направленность партикла
    Fvector my_head_pos;
    my_head_pos.set(get_head_position(this));

    Fvector position;
    position.set(get_head_position(enemy));
    position.y -= 0.5f;

    Fvector dir;
    dir.sub(position, my_head_pos);
    dir.normalize();

    PlayParticles(particles_fire, my_head_pos, dir);

    // check probability
    /*if (Random.randI(100) > 30)*/
    {
        //		Hit_Psy						(enemy, m_tube_damage / 2.f);
        play_control_sound_hit();
    }

    // m_sound_hit_fx.set_volume(10.0f);
    // if(!m_sndShockEffector)
    //	m_sndShockEffector = new SndShockEffector();

    // m_sndShockEffector->Start(m_sound_hit_fx._handle()->length_ms(), 10.f );
    // m_sound_hit_fx.play_at_pos(this, Level().CurrentEntity()->Position());
}

void CController::psy_fire()
{
    if (!EnemyMan.get_enemy())
        return;

    draw_fire_particles();
    /*
        active_control_fx			= true;
        time_control_hit_started	= Device.dwTimeGlobal;
    */
}

bool CController::can_psy_fire()
{
    if (m_psy_fire_start_time + m_psy_fire_delay > time())
    {
        return false;
    }

    if (!EnemyMan.get_enemy())
    {
        return false;
    }

    if (!EnemyMan.see_enemy_now())
    {
        return false;
    }

    float cur_yaw = custom_dir().get_head_orientation().current.yaw;
    float dir_yaw = Fvector().sub(EnemyMan.get_enemy()->Position(), Position()).getH();
    dir_yaw = angle_normalize(-dir_yaw);

    if (angle_difference(cur_yaw, dir_yaw) > _pmt_psy_attack_min_angle)
    {
        return false;
    }

    m_psy_fire_start_time = time();
    return true;
}

void CController::set_psy_fire_delay_zero() { m_psy_fire_delay = 0; }
void CController::set_psy_fire_delay_default() { m_psy_fire_delay = _pmt_psy_attack_delay; }
//////////////////////////////////////////////////////////////////////////
// TUBE
//////////////////////////////////////////////////////////////////////////

void CController::tube_fire() { control().activate(ControlCom::eComCustom1); }
bool CController::can_tube_fire()
{
    using namespace detail::controller;

    if (0 && m_tube_at_once)
    {
        if (EnemyMan.get_enemy() && EnemyMan.see_enemy_now() && m_psy_hit->check_start_conditions())
        {
            return true;
        }

        return false;
    }

    if (!EnemyMan.get_enemy())
        return false;

    if (EnemyMan.see_enemy_duration() < m_tube_condition_see_duration)
        return false;

    if (!m_psy_hit->check_start_conditions())
        return false;

    if (EnemyMan.get_enemy()->Position().distance_to(Position()) < m_tube_condition_min_distance)
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////

const MonsterSpace::SBoneRotation& CController::head_orientation() const
{
    return m_custom_dir_base->get_head_orientation();
}

void CController::test_covers()
{
    //////////////////////////////////////////////////////////////////////////
    // update covers
    //////////////////////////////////////////////////////////////////////////
}

void CController::create_base_controls()
{
    m_custom_anim_base = new CControllerAnimation();
    m_custom_dir_base = new CControllerDirection();

    m_anim_base = m_custom_anim_base;
    m_dir_base = m_custom_dir_base;

    m_move_base = new CControlMovementBase();
    m_path_base = new CControlPathBuilderBase();
}

void CController::TranslateActionToPathParams()
{
    // if (m_mental_state == eStateIdle) {
    //	inherited::TranslateActionToPathParams();
    //	return;
    //}
    // custom_anim().set_path_params();

    if ((anim().m_tAction != ACT_RUN) && (anim().m_tAction != ACT_WALK_FWD))
    {
        inherited::TranslateActionToPathParams();
        return;
    }

    u32 vel_mask = (m_bDamaged ? MonsterMovement::eVelocityParamsWalkDamaged : MonsterMovement::eVelocityParamsWalk);
    u32 des_mask =
        (m_bDamaged ? MonsterMovement::eVelocityParameterWalkDamaged : MonsterMovement::eVelocityParameterWalkNormal);

    if (m_force_real_speed)
        vel_mask = des_mask;

    path().set_velocity_mask(vel_mask);
    path().set_desirable_mask(des_mask);
    path().enable_path();
}

bool CController::is_relation_enemy(const CEntityAlive* tpEntityAlive) const
{
    //	MONSTER_COMMUNITY_ID
    if (xr_strcmp(*(tpEntityAlive->cNameSect()), "stalker_zombied") == 0)
        return false;
    if (is_community_friend_overrides(tpEntityAlive))
        return false;

    return inherited::is_relation_enemy(tpEntityAlive);
}

void CController::set_mental_state(EMentalState state)
{
    if (m_mental_state == state)
        return;

    m_mental_state = state;

    m_custom_anim_base->on_switch_controller();
}

void CController::HitEntity(
    const CEntity* pEntity, float fDamage, float impulse, Fvector& dir, ALife::EHitType hit_type, bool draw_hit_marks)
{
    if (pEntity == Actor() && !GodMode())
    {
        Actor()->conditions().PowerHit(m_stamina_hit, false);
        if (Actor()->conditions().GetPower() < m_stamina_hit)
        {
            if (!Actor()->inventory().Action((u16)kDROP, CMD_STOP))
            {
                Actor()->g_PerformDrop();
            }
        }
    }

    inherited::HitEntity(pEntity, fDamage, impulse, dir, hit_type, draw_hit_marks);
}

bool CController::tube_ready() const { return m_psy_hit && m_psy_hit->tube_ready(); }
#ifdef DEBUG
CBaseMonster::SDebugInfo CController::show_debug_info()
{
    CBaseMonster::SDebugInfo info = inherited::show_debug_info();
    if (!info.active)
        return CBaseMonster::SDebugInfo();

    // Draw Controlled Lines
    DBG().level_info(this).clear();

    Fvector my_pos = Position();
    my_pos.y += 1.5f;

    for (u32 i = 0; i < m_controlled_objects.size(); i++)
    {
        Fvector enemy_pos = m_controlled_objects[i]->Position();

        Fvector dir;
        dir.sub(enemy_pos, Position());
        dir.div(2.f);
        Fvector new_pos;
        new_pos.add(Position(), dir);
        new_pos.y += 10.f;

        enemy_pos.y += 1.0f;

        DBG().level_info(this).add_item(my_pos, new_pos, color_xrgb(0, 255, 255));
        DBG().level_info(this).add_item(enemy_pos, new_pos, color_xrgb(0, 255, 255));
    }

    return CBaseMonster::SDebugInfo();
}
#endif

#ifdef _DEBUG
void CController::debug_on_key(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_MINUS:
        // m_sound_aura_left_channel.play_at_pos(Level().CurrentEntity(), Fvector().set(-1.f, 0.f, 1.f), sm_2D);
        // m_sound_aura_right_channel.play_at_pos(Level().CurrentEntity(), Fvector().set(1.f, 0.f, 1.f), sm_2D);

        if (m_psy_hit->check_start_conditions())
        {
            control().activate(ControlCom::eComCustom1);
        }
        // P1.set		(Actor()->Position());
        //
        // DBG().level_info(this).remove_item	(u32(0));
        // DBG().level_info(this).add_item(P1,0.5f,COLOR_BLUE,0);

        // if (!fsimilar(P1.square_magnitude(),0.f) &&
        //	!fsimilar(P2.square_magnitude(),0.f)) {
        //	const CCoverPoint *cover = CoverMan->find_cover(P1,P2,10.f,40.f);
        //	if (cover) {
        //		DBG().level_info(this).remove_item	(3);
        //		DBG().level_info(this).add_item		(cover->position(),0.8f,COLOR_RED,3);
        //	}
        //}

        break;
    case SDL_SCANCODE_EQUALS:
        P2.set(Actor()->Position());
        DBG().level_info(this).remove_item(1);
        DBG().level_info(this).add_item(P2, 0.5f, COLOR_GREEN, 1);

        if (!fsimilar(P1.square_magnitude(), 0.f) && !fsimilar(P2.square_magnitude(), 0.f))
        {
            const CCoverPoint* cover = CoverMan->find_cover(P1, P2, 10.f, 40.f);
            if (cover)
            {
                DBG().level_info(this).remove_item(3);
                DBG().level_info(this).add_item(cover->position(), 0.8f, COLOR_RED, 3);
            }
        }

        // m_sound_aura_hit_left_channel.play_at_pos(Level().CurrentEntity(), Fvector().set(-1.f, 0.f, 1.f), sm_2D);
        // m_sound_aura_hit_right_channel.play_at_pos(Level().CurrentEntity(), Fvector().set(1.f, 0.f, 1.f), sm_2D);
        break;
    }
}
#endif
