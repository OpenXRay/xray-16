#include "StdAfx.h"
#include "base_monster.h"
#include "ai_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"
#include "PHMovementControl.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/level_graph.h"
#include "ai/monsters/corpse_cover.h"
#include "cover_evaluators.h"
#include "sound_player.h"
#include "ai/monsters/state_manager.h"
#include "ai/monsters/controlled_entity.h"
#include "ai/monsters/anomaly_detector.h"
#include "ai/monsters/monster_cover_manager.h"
#include "ai/monsters/monster_home.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Level.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "xrServer.h"
#include "inventory_item.h"
#include "xrServerEntities/xrServer_Objects_ALife.h"
#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/monster_velocity_space.h"
#include "ai/monsters/anti_aim_ability.h"

namespace detail
{
namespace base_monster
{
const float feel_enemy_who_just_hit_max_distance = 20;
const float feel_enemy_max_distance = 3;
const float feel_enemy_who_made_sound_max_distance = 49;

const float aom_far_radius = 9;
const float aom_prepare_radius = 7;
const float aom_prepare_time = 0;
const float aom_attack_radius = 0.6f;
const float aom_update_side_period = 4000;
const float aom_prediction_factor = 1.3f;

} // namespace base_monster

} // namespace detail

void CBaseMonster::Load(LPCSTR section)
{
    m_section = section;
    // load parameters from ".ltx" file
    inherited::Load(section);

    m_head_bone_name = READ_IF_EXISTS(pSettings, r_string, section, "bone_head", "bip01_head");
    m_left_eye_bone_name = READ_IF_EXISTS(pSettings, r_string, section, "bone_eye_left", 0);
    m_right_eye_bone_name = READ_IF_EXISTS(pSettings, r_string, section, "bone_eye_right", 0);

    m_corpse_cover_evaluator = new CMonsterCorpseCoverEvaluator(&movement().restrictions());
    m_enemy_cover_evaluator = new CCoverEvaluatorFarFromEnemy(&movement().restrictions());
    m_cover_evaluator_close_point = new CCoverEvaluatorCloseToEnemy(&movement().restrictions());

    MeleeChecker.load(section);
    Morale.load(section);

    m_pPhysics_support->in_Load(section);

    SetfHealth((float)pSettings->r_u32(section, "Health"));

    m_controlled = smart_cast<CControlledEntityBase*>(this);

    settings_load(section);

    control().load(section);

    m_anomaly_detector->load(section);
    CoverMan->load();

    m_rank = (pSettings->line_exist(section, "rank")) ? int(pSettings->r_u32(section, "rank")) : 0;

    //	if (pSettings->line_exist(section,"Spawn_Inventory_Item_Section")) {
    //		m_item_section					= pSettings->r_string(section,"Spawn_Inventory_Item_Section");
    //		m_spawn_probability				= pSettings->r_float(section,"Spawn_Inventory_Item_Probability");
    //	} else m_spawn_probability			= 0.f;

    m_melee_rotation_factor = READ_IF_EXISTS(pSettings, r_float, section, "Melee_Rotation_Factor", 1.5f);
    berserk_always = !!READ_IF_EXISTS(pSettings, r_bool, section, "berserk_always", false);

    m_feel_enemy_who_just_hit_max_distance = READ_IF_EXISTS(pSettings, r_float, section,
        "feel_enemy_who_just_hit_max_distance", detail::base_monster::feel_enemy_who_just_hit_max_distance);

    m_feel_enemy_max_distance = READ_IF_EXISTS(
        pSettings, r_float, section, "feel_enemy_max_distance", detail::base_monster::feel_enemy_max_distance);

    m_feel_enemy_who_made_sound_max_distance = READ_IF_EXISTS(pSettings, r_float, section,
        "feel_enemy_who_made_sound_max_distance", detail::base_monster::feel_enemy_who_made_sound_max_distance);

    //------------------------------------
    // Steering Behaviour
    //------------------------------------
    float separate_factor = READ_IF_EXISTS(pSettings, r_float, section, "separate_factor", 0.f);
    float separate_range = READ_IF_EXISTS(pSettings, r_float, section, "separate_range", 0.f);

    if ((separate_factor > 0.0001f) && (separate_range > 0.01f))
    {
        m_steer_manager = new steering_behaviour::manager();

        m_grouping_behaviour = new squad_grouping_behaviour(
            this, Fvector3().set(0.f, 0.f, 0.f), Fvector3().set(0.f, separate_factor, 0.f), separate_range);

        get_steer_manager()->add(new steering_behaviour::grouping(m_grouping_behaviour));
    }

    //------------------------------------
    // Auras
    //------------------------------------

    m_psy_aura.load_from_ini(pSettings, section);
    m_radiation_aura.load_from_ini(pSettings, section, true);
    m_fire_aura.load_from_ini(pSettings, section);
    m_base_aura.load_from_ini(pSettings, section);

    //------------------------------------
    // Protections
    //------------------------------------
    m_fSkinArmor = 0.f;
    m_fHitFracMonster = 0.1f;
    if (pSettings->line_exist(section, "protections_sect"))
    {
        LPCSTR protections_sect = pSettings->r_string(section, "protections_sect");
        m_fSkinArmor = READ_IF_EXISTS(pSettings, r_float, protections_sect, "skin_armor", 0.f);
        m_fHitFracMonster = READ_IF_EXISTS(pSettings, r_float, protections_sect, "hit_fraction_monster", 0.1f);
    }

    m_force_anti_aim = false;
}

void CBaseMonster::PostLoad(LPCSTR section)
{
    //------------------------------------
    // Atack On Move (AOM) Parameters
    //------------------------------------
    attack_on_move_params_t& aom = m_attack_on_move_params;

    aom.enabled = (READ_IF_EXISTS(pSettings, r_bool, section, "aom_enabled", FALSE)) != 0;
    aom.far_radius =
        READ_IF_EXISTS(pSettings, r_float, section, "aom_far_radius", detail::base_monster::aom_far_radius);
    aom.attack_radius =
        READ_IF_EXISTS(pSettings, r_float, section, "aom_attack_radius", detail::base_monster::aom_attack_radius);
    aom.update_side_period = READ_IF_EXISTS(
        pSettings, r_float, section, "aom_update_side_period", detail::base_monster::aom_update_side_period);
    aom.prediction_factor = READ_IF_EXISTS(
        pSettings, r_float, section, "aom_prediction_factor", detail::base_monster::aom_prediction_factor);
    aom.prepare_time =
        READ_IF_EXISTS(pSettings, r_float, section, "aom_prepare_time", detail::base_monster::aom_prepare_time);
    aom.prepare_radius =
        READ_IF_EXISTS(pSettings, r_float, section, "aom_prepare_radius", detail::base_monster::aom_prepare_radius);
    aom.max_go_close_time = READ_IF_EXISTS(pSettings, r_float, section, "aom_max_go_close_time", 8.f);

    if (aom.enabled)
    {
        SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);

        pcstr attack_on_move_anim_l =
            READ_IF_EXISTS(pSettings, r_string, section, "aom_animation_left", "stand_attack_run_");
        anim().AddAnim(eAnimAttackOnRunLeft, attack_on_move_anim_l, -1, &velocity_run, PS_STAND);
        pcstr attack_on_move_anim_r =
            READ_IF_EXISTS(pSettings, r_string, section, "aom_animation_right", "stand_attack_run_");
        anim().AddAnim(eAnimAttackOnRunRight, attack_on_move_anim_r, -1, &velocity_run, PS_STAND);
    }

    //------------------------------------
    // Anti-Aim ability
    //------------------------------------
    if (pSettings->line_exist(section, "anti_aim_effectors"))
    {
        SVelocityParam& velocity_stand = move().get_velocity(MonsterMovement::eVelocityParameterStand);

        m_anti_aim = new anti_aim_ability(this);
        control().add(m_anti_aim, ControlCom::eAntiAim);

        pcstr anti_aim_animation = READ_IF_EXISTS(pSettings, r_string, section, "anti_aim_animation", "stand_attack_");
        anim().AddAnim(eAnimAntiAimAbility, anti_aim_animation, -1, &velocity_stand, PS_STAND);
        m_anti_aim->load_from_ini(pSettings, section);
    }
}

steering_behaviour::manager* CBaseMonster::get_steer_manager()
{
    VERIFY(m_steer_manager);
    return m_steer_manager;
}

// if sound is absent just do not load that one
#define LOAD_SOUND(sound_name, _type, _prior, _mask, _int_type)                                                \
    if (pSettings->line_exist(section, sound_name))                                                            \
        sound().add(pSettings->r_string(section, sound_name), DEFAULT_SAMPLE_COUNT, _type, _prior, u32(_mask), \
            _int_type, m_head_bone_name);

void CBaseMonster::reload(LPCSTR section)
{
    CCustomMonster::reload(section);

    if (!CCustomMonster::use_simplified_visual())
        CStepManager::reload(section);

    movement().reload(section);

    // load base sounds
    LOAD_SOUND("sound_idle", SOUND_TYPE_MONSTER_TALKING, MonsterSound::eLowPriority, MonsterSound::eBaseChannel,
        MonsterSound::eMonsterSoundIdle);
    LOAD_SOUND("sound_distant_idle", SOUND_TYPE_MONSTER_TALKING, MonsterSound::eLowPriority + 1,
        MonsterSound::eBaseChannel, MonsterSound::eMonsterSoundIdleDistant);
    LOAD_SOUND("sound_eat", SOUND_TYPE_MONSTER_TALKING, MonsterSound::eNormalPriority + 4, MonsterSound::eBaseChannel,
        MonsterSound::eMonsterSoundEat);
    LOAD_SOUND("sound_aggressive", SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eNormalPriority + 3,
        MonsterSound::eBaseChannel, MonsterSound::eMonsterSoundAggressive);
    LOAD_SOUND("sound_attack_hit", SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eHighPriority + 1,
        MonsterSound::eCaptureAllChannels, MonsterSound::eMonsterSoundAttackHit);
    LOAD_SOUND("sound_take_damage", SOUND_TYPE_MONSTER_INJURING, MonsterSound::eHighPriority,
        MonsterSound::eCaptureAllChannels, MonsterSound::eMonsterSoundTakeDamage);
    LOAD_SOUND("sound_strike", SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eNormalPriority,
        MonsterSound::eChannelIndependent, MonsterSound::eMonsterSoundStrike);
    LOAD_SOUND("sound_die", SOUND_TYPE_MONSTER_DYING, MonsterSound::eCriticalPriority,
        MonsterSound::eCaptureAllChannels, MonsterSound::eMonsterSoundDie);
    LOAD_SOUND("sound_die_in_anomaly", SOUND_TYPE_MONSTER_DYING, MonsterSound::eCriticalPriority,
        MonsterSound::eCaptureAllChannels, MonsterSound::eMonsterSoundDieInAnomaly);
    LOAD_SOUND("sound_threaten", SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eNormalPriority,
        MonsterSound::eBaseChannel, MonsterSound::eMonsterSoundThreaten);
    LOAD_SOUND("sound_steal", SOUND_TYPE_MONSTER_STEP, MonsterSound::eNormalPriority + 1, MonsterSound::eBaseChannel,
        MonsterSound::eMonsterSoundSteal);
    LOAD_SOUND("sound_panic", SOUND_TYPE_MONSTER_STEP, MonsterSound::eNormalPriority + 2, MonsterSound::eBaseChannel,
        MonsterSound::eMonsterSoundPanic);

    control().reload(section);

    // load monster type
    m_monster_type = eMonsterTypeUniversal;
    if (pSettings->line_exist(section, "monster_type"))
    {
        if (xr_strcmp(pSettings->r_string(section, "monster_type"), "indoor") == 0)
            m_monster_type = eMonsterTypeIndoor;
        else if (xr_strcmp(pSettings->r_string(section, "monster_type"), "outdoor") == 0)
            m_monster_type = eMonsterTypeOutdoor;
    }

    Home->load("home");

    // save panic_threshold
    m_default_panic_threshold = m_panic_threshold;
}

void CBaseMonster::reinit()
{
    inherited::reinit();

    EnemyMemory.clear();
    SoundMemory.clear();
    CorpseMemory.clear();
    HitMemory.clear();

    EnemyMan.reinit();
    CorpseMan.reinit();

    StateMan->reinit();

    Morale.reinit();

    m_bDamaged = false;
    m_bAngry = false;
    m_bAggressive = false;
    m_bSleep = false;
    m_bRunTurnLeft = false;
    m_bRunTurnRight = false;

    state_invisible = false;

    m_force_real_speed = false;
    m_script_processing_active = false;

    if (m_controlled)
        m_controlled->on_reinit();

    ignore_collision_hit = false;

    control().reinit();

    m_anomaly_detector->reinit();

    m_skip_transfer_enemy = false;

    MeleeChecker.init_attack();

    time_berserk_start = 0;

    m_prev_sound_type = MonsterSound::eMonsterSoundIdle;

#ifdef DEBUG
    m_show_debug_info = 0;
#endif

    m_offset_from_leader_chosen_tick = 0;
    m_offset_from_leader = Fvector().set(0.f, 0.f, 0.f);

    m_action_target_node = u32(-1);

    m_first_tick_enemy_inaccessible = 0;
    m_last_tick_enemy_inaccessible = 0;
    m_first_tick_object_not_at_home = 0;

    anim().clear_override_animation();
}

BOOL CBaseMonster::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);

    CSE_Abstract* e = (CSE_Abstract*)(DC);
    R_ASSERT2(ai().get_level_graph() && ai().get_cross_table() && (ai().level_graph().level_id() != u32(-1)),
        "There is no AI-Map, level graph, cross table, or graph is not compiled into the game graph!");
    monster_squad().register_member((u8)g_Team(), (u8)g_Squad(), (u8)g_Group(), this);
    settings_overrides();

    if (GetScriptControl())
    {
        m_control_manager->animation().reset_data();
        ProcessScripts();
    }
    m_pPhysics_support->in_NetSpawn(e);

    control().update_frame();
    control().update_schedule();

    // spawn inventory item
    //	if (ai().get_alife()) {
    //
    //		CSE_ALifeMonsterBase					*se_monster =
    // smart_cast<CSE_ALifeMonsterBase*>(ai().alife().objects().object(ID()));
    //		VERIFY									(se_monster);
    //
    //		if (se_monster->m_flags.is(CSE_ALifeMonsterBase::flNeedCheckSpawnItem)) {
    //			float prob = Random.randF();
    //			if ((prob < m_spawn_probability) || fsimilar(m_spawn_probability,1.f))
    //				se_monster->m_flags.set(CSE_ALifeMonsterBase::flSkipSpawnItem, FALSE);
    //
    //			se_monster->m_flags.set(CSE_ALifeMonsterBase::flNeedCheckSpawnItem, FALSE);
    //		}
    //
    //		if (!se_monster->m_flags.is(CSE_ALifeMonsterBase::flSkipSpawnItem)) {
    //			CSE_Abstract	*object = Level().spawn_item
    //(m_item_section,Position(),ai_location().level_vertex_id(),ID(),true);
    //			CSE_ALifeObject	*alife_object = smart_cast<CSE_ALifeObject*>(object);
    //			if (alife_object)
    //				alife_object->m_flags.set	(CSE_ALifeObject::flCanSave,FALSE);
    //
    //			{
    //				NET_Packet				P;
    //				object->Spawn_Write		(P,TRUE);
    //				Level().Send			(P,net_flags(TRUE));
    //				F_entity_Destroy		(object);
    //			}
    //		}
    //	}

    return (TRUE);
}

void CBaseMonster::net_Destroy()
{
    // функция должена быть вызвана перед inherited
    if (m_controlled)
        m_controlled->on_destroy();
    if (StateMan)
        StateMan->critical_finalize();

    inherited::net_Destroy();

    m_pPhysics_support->in_NetDestroy();

    monster_squad().remove_member((u8)g_Team(), (u8)g_Squad(), (u8)g_Group(), this);

#ifdef DEBUG
    m_show_debug_info = 0;
#endif
}

#define READ_SETTINGS(var, name, method, ltx, section) \
    {                                                  \
        if (ltx == pSettings)                          \
            var = ltx->method(section, name);          \
        else if (ltx->line_exist(section, name))       \
            var = ltx->method(section, name);          \
    \
}

void CBaseMonster::settings_read(CInifile const* ini, LPCSTR section, SMonsterSettings& data)
{
    READ_SETTINGS(data.m_fSoundThreshold, "SoundThreshold", r_float, ini, section);

    if (ability_run_attack())
    {
        READ_SETTINGS(data.m_run_attack_path_dist, "RunAttack_PathDistance", r_float, ini, section);
        READ_SETTINGS(data.m_run_attack_start_dist, "RunAttack_StartDistance", r_float, ini, section);
    }

    READ_SETTINGS(data.m_dwDayTimeBegin, "DayTime_Begin", r_u32, ini, section);
    READ_SETTINGS(data.m_dwDayTimeEnd, "DayTime_End", r_u32, ini, section);

    READ_SETTINGS(data.m_fDistToCorpse, "distance_to_corpse", r_float, ini, section);
    READ_SETTINGS(data.satiety_threshold, "satiety_threshold", r_float, ini, section);

    READ_SETTINGS(data.m_fDamagedThreshold, "DamagedThreshold", r_float, ini, section);

    READ_SETTINGS(data.m_dwIdleSndDelay, "idle_sound_delay", r_u32, ini, section);
    READ_SETTINGS(data.m_dwEatSndDelay, "eat_sound_delay", r_u32, ini, section);
    READ_SETTINGS(data.m_dwAttackSndDelay, "attack_sound_delay", r_u32, ini, section);

    READ_SETTINGS(data.m_dwDistantIdleSndDelay, "distant_idle_sound_delay", r_u32, ini, section);
    READ_SETTINGS(data.m_fDistantIdleSndRange, "distant_idle_sound_range", r_float, ini, section);

    READ_SETTINGS(data.m_fEatFreq, "eat_freq", r_float, ini, section);
    READ_SETTINGS(data.m_fEatSlice, "eat_slice", r_float, ini, section);
    READ_SETTINGS(data.m_fEatSliceWeight, "eat_slice_weight", r_float, ini, section);

    READ_SETTINGS(data.m_legs_number, "LegsCount", r_u8, ini, section);
    READ_SETTINGS(data.m_max_hear_dist, "max_hear_dist", r_float, ini, section);

    // Load attack postprocess
    if (ini->line_exist(section, "attack_effector"))
    {
        LPCSTR ppi_section = ini->r_string(section, "attack_effector");

        READ_SETTINGS(data.m_attack_effector.ppi.duality.h, "duality_h", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.duality.v, "duality_v", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.gray, "gray", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.blur, "blur", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.noise.intensity, "noise_intensity", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.noise.grain, "noise_grain", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ppi.noise.fps, "noise_fps", r_float, ini, ppi_section);

        VERIFY(!fis_zero(data.m_attack_effector.ppi.noise.fps));

        if (ini->line_exist(ppi_section, "color_base"))
            sscanf(ini->r_string(ppi_section, "color_base"), "%f,%f,%f", &data.m_attack_effector.ppi.color_base.r,
                &data.m_attack_effector.ppi.color_base.g, &data.m_attack_effector.ppi.color_base.b);
        if (ini->line_exist(ppi_section, "color_gray"))
            sscanf(ini->r_string(ppi_section, "color_gray"), "%f,%f,%f", &data.m_attack_effector.ppi.color_gray.r,
                &data.m_attack_effector.ppi.color_gray.g, &data.m_attack_effector.ppi.color_gray.b);
        if (ini->line_exist(ppi_section, "color_add"))
            sscanf(ini->r_string(ppi_section, "color_add"), "%f,%f,%f", &data.m_attack_effector.ppi.color_add.r,
                &data.m_attack_effector.ppi.color_add.g, &data.m_attack_effector.ppi.color_add.b);

        READ_SETTINGS(data.m_attack_effector.time, "time", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.time_attack, "time_attack", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.time_release, "time_release", r_float, ini, ppi_section);

        READ_SETTINGS(data.m_attack_effector.ce_time, "ce_time", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ce_amplitude, "ce_amplitude", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ce_period_number, "ce_period_number", r_float, ini, ppi_section);
        READ_SETTINGS(data.m_attack_effector.ce_power, "ce_power", r_float, ini, ppi_section);
    }
}

void CBaseMonster::settings_load(LPCSTR section)
{
    SMonsterSettings data;

    settings_read(pSettings, section, data);

    u32 crc = crc32(&data, sizeof(SMonsterSettings));
    m_base_settings.create(crc, 1, &data);
}

void CBaseMonster::settings_overrides()
{
    SMonsterSettings* data;
    data = *m_base_settings;

    if (spawn_ini() && spawn_ini()->section_exist("settings_overrides"))
    {
        settings_read(spawn_ini(), "settings_overrides", (*data));
    }

    u32 crc = crc32(data, sizeof(SMonsterSettings));
    m_current_settings.create(crc, 1, data);
}

void CBaseMonster::on_before_sell(CInventoryItem* item)
{
    // since there can be only single item in the monster inventory
    CSE_Abstract* object = Level().Server->GetGameState()->get_entity_from_eid(item->object().ID());
    VERIFY(object);
    CSE_ALifeObject* alife_object = smart_cast<CSE_ALifeObject*>(object);
    if (alife_object)
        alife_object->m_flags.set(CSE_ALifeObject::flCanSave, TRUE);
}

void CBaseMonster::load_critical_wound_bones()
{
    // animation does not exist - no bones loaded
    if (pSettings->line_exist(cNameSect(), "critical_wound_anim_head"))
    {
        fill_bones_body_parts("critical_wound_bones_head", critical_wound_type_head);
        m_critical_wound_anim_head = pSettings->r_string(cNameSect(), "critical_wound_anim_head");
    }

    if (pSettings->line_exist(cNameSect(), "critical_wound_anim_torso"))
    {
        fill_bones_body_parts("critical_wound_bones_torso", critical_wound_type_torso);
        m_critical_wound_anim_torso = pSettings->r_string(cNameSect(), "critical_wound_anim_torso");
    }

    if (pSettings->line_exist(cNameSect(), "critical_wound_anim_legs"))
    {
        fill_bones_body_parts("critical_wound_bones_legs", critical_wound_type_legs);
        m_critical_wound_anim_legs = pSettings->r_string(cNameSect(), "critical_wound_anim_legs");
    }
}

void CBaseMonster::fill_bones_body_parts(LPCSTR body_part, CriticalWoundType wound_type)
{
    LPCSTR body_parts_section = pSettings->r_string(cNameSect(), body_part);

    IKinematics* kinematics = smart_cast<IKinematics*>(Visual());
    VERIFY(kinematics);

    CInifile::Sect& body_part_section = pSettings->r_section(body_parts_section);
    auto I = body_part_section.Data.cbegin();
    auto E = body_part_section.Data.cend();
    for (; I != E; ++I)
        m_bones_body_parts.insert(std::make_pair(kinematics->LL_BoneID((*I).first), u32(wound_type)));
}
