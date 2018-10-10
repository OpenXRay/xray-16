////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker.cpp
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "ai/ai_monsters_misc.h"
#include "Weapon.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"
#include "script_entity_action.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/game_graph.h"
#include "Inventory.h"
#include "Artefact.h"
#include "PHMovementControl.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "cover_evaluators.h"
#include "xrServer.h"
#include "xr_level_controller.h"
#include "Include/xrRender/Kinematics.h"
#include "xrServerEntities/character_info.h"
#include "Actor.h"
#include "relation_registry.h"
#include "stalker_animation_manager.h"
#include "stalker_planner.h"
#include "script_game_object.h"
#include "detail_path_manager.h"
#include "agent_manager.h"
#include "agent_corpse_manager.h"
#include "object_handler_planner.h"
#include "object_handler_space.h"
#include "memory_manager.h"
#include "sight_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/ai_object_location_impl.h"
#include "stalker_movement_manager_smart_cover.h"
#include "EntityCondition.h"
#include "xrScriptEngine/script_engine.hpp"
#include "ai_stalker_impl.h"
#include "sound_player.h"
#include "stalker_sound_data.h"
#include "stalker_sound_data_visitor.h"
#include "ai_stalker_space.h"
#include "mt_config.h"
#include "EffectorShot.h"
#include "visual_memory_manager.h"
#include "enemy_manager.h"
#include "xrServerEntities/alife_human_brain.h"
#include "xrEngine/profiler.h"
#include "BoneProtections.h"
#include "stalker_animation_names.h"
#include "stalker_decision_space.h"
#include "agent_member_manager.h"
#include "location_manager.h"
#include "smart_cover_animation_selector.h"
#include "smart_cover_animation_planner.h"
#include "smart_cover_planner_target_selector.h"

#ifdef DEBUG
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "Level.h"
#include "map_location.h"
#include "map_manager.h"
#endif // DEBUG

using namespace StalkerSpace;

extern int g_AI_inactive_time;

CAI_Stalker::CAI_Stalker()
    : m_sniper_update_rate(false), m_sniper_fire_mode(false), m_take_items_enabled(true), m_death_sound_enabled(true)
{
    m_sound_user_data_visitor = 0;
    m_movement_manager = 0;
    m_group_behaviour = true;
    m_boneHitProtection = NULL;
    m_power_fx_factor = flt_max;
    m_wounded = false;
#ifdef DEBUG
    m_debug_planner = 0;
    m_dbg_hud_draw = false;
#endif // DEBUG
    m_registered_in_combat_on_migration = false;
}

CAI_Stalker::~CAI_Stalker()
{
    xr_delete(m_pPhysics_support);
    xr_delete(m_animation_manager);
    xr_delete(m_brain);
    xr_delete(m_sight_manager);
    xr_delete(m_weapon_shot_effector);
    xr_delete(m_sound_user_data_visitor);
}

void CAI_Stalker::reinit()
{
    CObjectHandler::reinit(this);
    sight().reinit();
    CCustomMonster::reinit();
    animation().reinit();
    //	movement().reinit				();

    //загрузка спецевической звуковой схемы для сталкера согласно m_SpecificCharacter
    sound().sound_prefix(SpecificCharacter().sound_voice_prefix());

    LoadSounds(*cNameSect());

    m_pPhysics_support->in_Init();

    m_best_item_to_kill = 0;
    m_best_item_value = 0.f;
    m_best_ammo = 0;
    m_best_found_item_to_kill = 0;
    m_best_found_ammo = 0;
    m_item_actuality = false;
    m_sell_info_actuality = false;

    m_ce_close = new CCoverEvaluatorCloseToEnemy(&movement().restrictions());
    m_ce_far = new CCoverEvaluatorFarFromEnemy(&movement().restrictions());
    m_ce_best = new CCoverEvaluatorBest(&movement().restrictions());
    m_ce_angle = new CCoverEvaluatorAngle(&movement().restrictions());
    m_ce_safe = new CCoverEvaluatorSafe(&movement().restrictions());
    m_ce_ambush = new CCoverEvaluatorAmbush(&movement().restrictions());

    m_ce_close->set_inertia(3000);
    m_ce_far->set_inertia(3000);
    m_ce_best->set_inertia(1000);
    m_ce_angle->set_inertia(5000);
    m_ce_safe->set_inertia(1000);
    m_ce_ambush->set_inertia(3000);

    m_can_kill_enemy = false;
    m_can_kill_member = false;
    m_pick_distance = 0.f;
    m_pick_frame_id = 0;

    m_weapon_shot_random_seed = s32(Level().timeServer_Async());

    m_best_cover = 0;
    m_best_cover_actual = false;
    m_best_cover_value = flt_max;

    m_throw_actual = false;
    m_computed_object_position = Fvector().set(flt_max, flt_max, flt_max);
    m_computed_object_direction = Fvector().set(flt_max, flt_max, flt_max);

    m_throw_target_position = Fvector().set(flt_max, flt_max, flt_max);
    m_throw_ignore_object = 0;

    m_throw_position = Fvector().set(flt_max, flt_max, flt_max);
    m_throw_velocity = Fvector().set(flt_max, flt_max, flt_max);

    m_throw_collide_position = Fvector().set(flt_max, flt_max, flt_max);
    m_throw_enabled = false;

    m_last_throw_time = 0;

    m_can_throw_grenades = true;
    m_throw_time_interval = 20000;

    brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, false);

    {
        m_critical_wound_weights.clear();
        //		LPCSTR							weights = pSettings->r_string(cNameSect(),"critical_wound_weights");
        LPCSTR weights = SpecificCharacter().critical_wound_weights();
        string16 temp;
        for (int i = 0, n = _GetItemCount(weights); i < n; ++i)
            m_critical_wound_weights.push_back((float)atof(_GetItem(weights, i, temp)));
    }

    m_update_rotation_on_frame = false;
}

void CAI_Stalker::LoadSounds(LPCSTR section)
{
    LPCSTR head_bone_name = pSettings->r_string(section, "bone_head");
    sound().add(pSettings->r_string(section, "sound_death"), 100, SOUND_TYPE_MONSTER_DYING, 0,
        u32(eStalkerSoundMaskDie), eStalkerSoundDie, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_anomaly_death"), 100, SOUND_TYPE_MONSTER_DYING, 0,
        u32(eStalkerSoundMaskDieInAnomaly), eStalkerSoundDieInAnomaly, head_bone_name, 0);
    sound().add(pSettings->r_string(section, "sound_hit"), 100, SOUND_TYPE_MONSTER_INJURING, 1,
        u32(eStalkerSoundMaskInjuring), eStalkerSoundInjuring, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_friendly_fire"), 100, SOUND_TYPE_MONSTER_INJURING, 1,
        u32(eStalkerSoundMaskInjuringByFriend), eStalkerSoundInjuringByFriend, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_panic_human"), 100, SOUND_TYPE_MONSTER_TALKING, 2,
        u32(eStalkerSoundMaskPanicHuman), eStalkerSoundPanicHuman, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_panic_monster"), 100, SOUND_TYPE_MONSTER_TALKING, 2,
        u32(eStalkerSoundMaskPanicMonster), eStalkerSoundPanicMonster, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_grenade_alarm"), 100, SOUND_TYPE_MONSTER_TALKING, 3,
        u32(eStalkerSoundMaskGrenadeAlarm), eStalkerSoundGrenadeAlarm, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_friendly_grenade_alarm"), 100, SOUND_TYPE_MONSTER_TALKING, 3,
        u32(eStalkerSoundMaskFriendlyGrenadeAlarm), eStalkerSoundFriendlyGrenadeAlarm, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_tolls"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eStalkerSoundMaskTolls), eStalkerSoundTolls, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_wounded"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eStalkerSoundMaskWounded), eStalkerSoundWounded, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_alarm"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskAlarm), eStalkerSoundAlarm, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_attack_no_allies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskAttackNoAllies), eStalkerSoundAttackNoAllies, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_attack_allies_single_enemy"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskAttackAlliesSingleEnemy), eStalkerSoundAttackAlliesSingleEnemy, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_attack_allies_several_enemies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskAttackAlliesSeveralEnemies), eStalkerSoundAttackAlliesSeveralEnemies, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_backup"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskBackup), eStalkerSoundBackup, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_detour"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskDetour), eStalkerSoundDetour, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_search1_no_allies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskSearch1NoAllies), eStalkerSoundSearch1NoAllies, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_search1_with_allies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskSearch1WithAllies), eStalkerSoundSearch1WithAllies, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_enemy_lost_no_allies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskEnemyLostNoAllies), eStalkerSoundEnemyLostNoAllies, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_enemy_lost_with_allies"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskEnemyLostWithAllies), eStalkerSoundEnemyLostWithAllies, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_humming"), 100, SOUND_TYPE_MONSTER_TALKING, 6,
        u32(eStalkerSoundMaskHumming), eStalkerSoundHumming, head_bone_name, 0);
    sound().add(pSettings->r_string(section, "sound_need_backup"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eStalkerSoundMaskNeedBackup), eStalkerSoundNeedBackup, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_running_in_danger"), 100, SOUND_TYPE_MONSTER_TALKING, 6,
        u32(eStalkerSoundMaskMovingInDanger), eStalkerSoundRunningInDanger, head_bone_name,
        new CStalkerSoundData(this));
    //	sound().add						(pSettings->r_string(section,"sound_walking_in_danger"),			100,
    // SOUND_TYPE_MONSTER_TALKING,	6, u32(eStalkerSoundMaskMovingInDanger),			eStalkerSoundWalkingInDanger,
    // head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_kill_wounded"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskKillWounded), eStalkerSoundKillWounded, head_bone_name, new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_enemy_critically_wounded"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eStalkerSoundMaskEnemyCriticallyWounded), eStalkerSoundEnemyCriticallyWounded, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_enemy_killed_or_wounded"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eStalkerSoundMaskEnemyKilledOrWounded), eStalkerSoundEnemyKilledOrWounded, head_bone_name,
        new CStalkerSoundData(this));
    sound().add(pSettings->r_string(section, "sound_throw_grenade"), 100, SOUND_TYPE_MONSTER_TALKING, 5,
        u32(eStalkerSoundMaskKillWounded), eStalkerSoundThrowGrenade, head_bone_name, new CStalkerSoundData(this));
}

void CAI_Stalker::reload(LPCSTR section)
{
    brain().setup(this);

    CCustomMonster::reload(section);
    if (!already_dead())
        CStepManager::reload(section);

    //	if (!already_dead())
    CObjectHandler::reload(section);

    if (!already_dead())
        sight().reload(section);

    if (!already_dead())
        movement().reload(section);

    m_disp_walk_stand = pSettings->r_float(section, "disp_walk_stand");
    m_disp_walk_crouch = pSettings->r_float(section, "disp_walk_crouch");
    m_disp_run_stand = pSettings->r_float(section, "disp_run_stand");
    m_disp_run_crouch = pSettings->r_float(section, "disp_run_crouch");
    m_disp_stand_stand = pSettings->r_float(section, "disp_stand_stand");
    m_disp_stand_crouch = pSettings->r_float(section, "disp_stand_crouch");
    m_disp_stand_stand_zoom = pSettings->r_float(section, "disp_stand_stand_zoom");
    m_disp_stand_crouch_zoom = pSettings->r_float(section, "disp_stand_crouch_zoom");

    m_can_select_weapon = true;

    LPCSTR queue_sect = pSettings->r_string(*cNameSect(), "fire_queue_section");
    if (xr_strcmp(queue_sect, "") && pSettings->section_exist(queue_sect))
    {
        m_pstl_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_size_far", 1);
        m_pstl_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_size_far", 1);
        m_pstl_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_interval_far", 1000);
        m_pstl_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_interval_far", 1250);

        m_pstl_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_size_medium", 2);
        m_pstl_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_size_medium", 4);
        m_pstl_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_interval_medium", 750);
        m_pstl_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_interval_medium", 1000);

        m_pstl_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_size_close", 3);
        m_pstl_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_size_close", 5);
        m_pstl_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_min_queue_interval_close", 500);
        m_pstl_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "pstl_max_queue_interval_close", 750);

        m_shtg_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_size_far", 1);
        m_shtg_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_size_far", 1);
        m_shtg_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_interval_far", 1250);
        m_shtg_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_interval_far", 1500);

        m_shtg_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_size_medium", 1);
        m_shtg_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_size_medium", 1);
        m_shtg_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_interval_medium", 750);
        m_shtg_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_interval_medium", 1250);

        m_shtg_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_size_close", 1);
        m_shtg_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_size_close", 1);
        m_shtg_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_min_queue_interval_close", 500);
        m_shtg_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "shtg_max_queue_interval_close", 1000);

        m_snp_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_size_far", 1);
        m_snp_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_size_far", 1);
        m_snp_min_queue_interval_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_interval_far", 3000);
        m_snp_max_queue_interval_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_interval_far", 4000);

        m_snp_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_size_medium", 1);
        m_snp_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_size_medium", 1);
        m_snp_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_interval_medium", 3000);
        m_snp_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_interval_medium", 4000);

        m_snp_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_size_close", 1);
        m_snp_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_size_close", 1);
        m_snp_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_min_queue_interval_close", 3000);
        m_snp_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "snp_max_queue_interval_close", 4000);

        m_mchg_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_size_far", 1);
        m_mchg_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_size_far", 6);
        m_mchg_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_interval_far", 500);
        m_mchg_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_interval_far", 1000);

        m_mchg_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_size_medium", 4);
        m_mchg_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_size_medium", 6);
        m_mchg_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_interval_medium", 500);
        m_mchg_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_interval_medium", 750);

        m_mchg_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_size_close", 4);
        m_mchg_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_size_close", 10);
        m_mchg_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_min_queue_interval_close", 300);
        m_mchg_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "mchg_max_queue_interval_close", 500);

        m_auto_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_size_far", 1);
        m_auto_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_size_far", 6);
        m_auto_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_interval_far", 500);
        m_auto_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_interval_far", 1000);

        m_auto_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_size_medium", 4);
        m_auto_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_size_medium", 6);
        m_auto_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_interval_medium", 500);
        m_auto_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_interval_medium", 750);

        m_auto_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_size_close", 4);
        m_auto_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_size_close", 10);
        m_auto_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_min_queue_interval_close", 300);
        m_auto_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, queue_sect, "auto_max_queue_interval_close", 500);

        //		m_pstl_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,queue_sect,"pstl_queue_fire_dist_close", 15.0f);
        m_pstl_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, queue_sect, "pstl_queue_fire_dist_med", 15.0f);
        m_pstl_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, queue_sect, "pstl_queue_fire_dist_far", 30.0f);
        //		m_shtg_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,queue_sect,"shtg_queue_fire_dist_close", 15.0f);
        m_shtg_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, queue_sect, "shtg_queue_fire_dist_med", 15.0f);
        m_shtg_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, queue_sect, "shtg_queue_fire_dist_far", 30.0f);
        //		m_snp_queue_fire_dist_close			=
        // READ_IF_EXISTS(pSettings,r_float,queue_sect,"snp_queue_fire_dist_close", 15.0f);
        m_snp_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, queue_sect, "snp_queue_fire_dist_med", 15.0f);
        m_snp_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, queue_sect, "snp_queue_fire_dist_far", 30.0f);
        //		m_mchg_queue_fire_dist_close			=
        // READ_IF_EXISTS(pSettings,r_float,queue_sect,"mchg_queue_fire_dist_close", 15.0f);
        m_mchg_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, queue_sect, "mchg_queue_fire_dist_med", 15.0f);
        m_mchg_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, queue_sect, "mchg_queue_fire_dist_far", 30.0f);
        //		m_auto_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,queue_sect,"auto_queue_fire_dist_close", 15.0f);
        m_auto_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, queue_sect, "auto_queue_fire_dist_med", 15.0f);
        m_auto_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, queue_sect, "auto_queue_fire_dist_far", 30.0f);
    }
    else
    {
        m_pstl_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_size_far", 1);
        m_pstl_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_size_far", 1);
        m_pstl_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_interval_far", 1000);
        m_pstl_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_interval_far", 1250);

        m_pstl_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_size_medium", 2);
        m_pstl_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_size_medium", 4);
        m_pstl_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_interval_medium", 750);
        m_pstl_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_interval_medium", 1000);

        m_pstl_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_size_close", 3);
        m_pstl_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_size_close", 5);
        m_pstl_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_min_queue_interval_close", 500);
        m_pstl_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "pstl_max_queue_interval_close", 750);

        m_shtg_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_size_far", 1);
        m_shtg_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_size_far", 1);
        m_shtg_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_interval_far", 1250);
        m_shtg_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_interval_far", 1500);

        m_shtg_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_size_medium", 1);
        m_shtg_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_size_medium", 1);
        m_shtg_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_interval_medium", 750);
        m_shtg_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_interval_medium", 1250);

        m_shtg_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_size_close", 1);
        m_shtg_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_size_close", 1);
        m_shtg_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_min_queue_interval_close", 500);
        m_shtg_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "shtg_max_queue_interval_close", 1000);

        m_snp_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_size_far", 1);
        m_snp_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_size_far", 1);
        m_snp_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_interval_far", 3000);
        m_snp_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_interval_far", 4000);

        m_snp_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_size_medium", 1);
        m_snp_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_size_medium", 1);
        m_snp_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_interval_medium", 3000);
        m_snp_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_interval_medium", 4000);

        m_snp_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_size_close", 1);
        m_snp_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_size_close", 1);
        m_snp_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_min_queue_interval_close", 3000);
        m_snp_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "snp_max_queue_interval_close", 4000);

        m_mchg_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_size_far", 1);
        m_mchg_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_size_far", 6);
        m_mchg_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_interval_far", 500);
        m_mchg_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_interval_far", 1000);

        m_mchg_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_size_medium", 4);
        m_mchg_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_size_medium", 6);
        m_mchg_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_interval_medium", 500);
        m_mchg_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_interval_medium", 750);

        m_mchg_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_size_close", 4);
        m_mchg_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_size_close", 10);
        m_mchg_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_min_queue_interval_close", 300);
        m_mchg_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "mchg_max_queue_interval_close", 500);

        m_auto_min_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_size_far", 1);
        m_auto_max_queue_size_far = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_size_far", 6);
        m_auto_min_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_interval_far", 500);
        m_auto_max_queue_interval_far =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_interval_far", 1000);

        m_auto_min_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_size_medium", 4);
        m_auto_max_queue_size_medium = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_size_medium", 6);
        m_auto_min_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_interval_medium", 500);
        m_auto_max_queue_interval_medium =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_interval_medium", 750);

        m_auto_min_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_size_close", 4);
        m_auto_max_queue_size_close = READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_size_close", 10);
        m_auto_min_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_min_queue_interval_close", 300);
        m_auto_max_queue_interval_close =
            READ_IF_EXISTS(pSettings, r_u32, *cNameSect(), "auto_max_queue_interval_close", 500);

        //		m_pstl_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,*cNameSect(),"pstl_queue_fire_dist_close", 15.0f);
        m_pstl_queue_fire_dist_med =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "pstl_queue_fire_dist_med", 15.0f);
        m_pstl_queue_fire_dist_far =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "pstl_queue_fire_dist_far", 30.0f);
        //		m_shtg_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,*cNameSect(),"shtg_queue_fire_dist_close", 15.0f);
        m_shtg_queue_fire_dist_med =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "shtg_queue_fire_dist_med", 15.0f);
        m_shtg_queue_fire_dist_far =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "shtg_queue_fire_dist_far", 30.0f);
        //		m_snp_queue_fire_dist_close			=
        // READ_IF_EXISTS(pSettings,r_float,*cNameSect(),"snp_queue_fire_dist_close", 15.0f);
        m_snp_queue_fire_dist_med = READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "snp_queue_fire_dist_med", 15.0f);
        m_snp_queue_fire_dist_far = READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "snp_queue_fire_dist_far", 30.0f);
        //		m_mchg_queue_fire_dist_close			=
        // READ_IF_EXISTS(pSettings,r_float,*cNameSect(),"mchg_queue_fire_dist_close", 15.0f);
        m_mchg_queue_fire_dist_med =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "mchg_queue_fire_dist_med", 15.0f);
        m_mchg_queue_fire_dist_far =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "mchg_queue_fire_dist_far", 30.0f);
        //		m_auto_queue_fire_dist_close		=
        // READ_IF_EXISTS(pSettings,r_float,**cNameSect(),"auto_queue_fire_dist_close", 15.0f);
        m_auto_queue_fire_dist_med =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "auto_queue_fire_dist_med", 15.0f);
        m_auto_queue_fire_dist_far =
            READ_IF_EXISTS(pSettings, r_float, *cNameSect(), "auto_queue_fire_dist_far", 30.0f);
    }
    m_power_fx_factor = pSettings->r_float(section, "power_fx_factor");
}

void CAI_Stalker::Die(IGameObject* who)
{
    movement().on_death();

    notify_on_wounded_or_killed(who);

    SelectAnimation(XFORM().k, movement().detail().direction(), movement().speed());

    if (m_death_sound_enabled)
    {
        sound().set_sound_mask((u32)eStalkerSoundMaskDie);
        if (is_special_killer(who))
            sound().play(eStalkerSoundDieInAnomaly);
        else
            sound().play(eStalkerSoundDie);
    }

    m_hammer_is_clutched = m_clutched_hammer_enabled &&
        !CObjectHandler::planner().m_storage.property(ObjectHandlerSpace::eWorldPropertyStrapped) &&
        !::Random.randI(0, 2);

    inherited::Die(who);

    //запретить использование слотов в инвенторе
    inventory().SetSlotsUseful(false);

    if (inventory().GetActiveSlot() == NO_ACTIVE_SLOT)
        return;

    CInventoryItem* active_item = inventory().ActiveItem();
    if (!active_item)
        return;

    CWeapon* weapon = smart_cast<CWeapon*>(active_item);
    if (!weapon)
        return;

    {
        TIItemContainer::iterator I = inventory().m_all.begin();
        TIItemContainer::iterator E = inventory().m_all.end();
        for (; I != E; ++I)
        {
            if (std::find(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end(), (*I)->object().cNameSect()) ==
                weapon->m_ammoTypes.end())
                continue;

            NET_Packet packet;
            u_EventGen(packet, GE_DESTROY, (*I)->object().ID());
            u_EventSend(packet);
        }
    }
}

void CAI_Stalker::Load(LPCSTR section)
{
    CCustomMonster::Load(section);
    CObjectHandler::Load(section);
    sight().Load(section);

    // skeleton physics
    m_pPhysics_support->in_Load(section);

    m_can_select_items = !!pSettings->r_bool(section, "can_select_items");
}

BOOL CAI_Stalker::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeHumanStalker* tpHuman = smart_cast<CSE_ALifeHumanStalker*>(e);
    R_ASSERT(tpHuman);

    // static bool first_time			= true;
    // if ( first_time ) {
    //	tpHuman->o_Position.z		-= 3.f;
    //	first_time					= false;
    //}

    m_group_behaviour = !!tpHuman->m_flags.test(CSE_ALifeObject::flGroupBehaviour);

    if (!CObjectHandler::net_Spawn(DC) || !inherited::net_Spawn(DC))
        return (FALSE);

    set_money(tpHuman->m_dwMoney, false);

    animation().reload();

    movement().m_head.current.yaw = movement().m_head.target.yaw = movement().m_body.current.yaw =
        movement().m_body.target.yaw = angle_normalize_signed(-tpHuman->o_torso.yaw);
    movement().m_body.current.pitch = movement().m_body.target.pitch = 0;

    if (ai().game_graph().valid_vertex_id(tpHuman->m_tGraphID))
        ai_location().game_vertex(tpHuman->m_tGraphID);

    if (ai().game_graph().valid_vertex_id(tpHuman->m_tNextGraphID) &&
        movement().restrictions().accessible(ai().game_graph().vertex(tpHuman->m_tNextGraphID)->level_point()))
        movement().set_game_dest_vertex(tpHuman->m_tNextGraphID);

    R_ASSERT2(ai().get_game_graph() && ai().get_level_graph() && ai().get_cross_table() &&
            (ai().level_graph().level_id() != u32(-1)),
        "There is no AI-Map, level graph, cross table, or graph is not compiled into the game graph!");

    setEnabled(TRUE);

    if (!Level().CurrentViewEntity())
        Level().SetEntity(this);

    if (!g_Alive())
        sound().set_sound_mask(u32(eStalkerSoundMaskDie));

    //загрузить иммунитеты из модельки сталкера
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    VERIFY(pKinematics);
    CInifile* ini = pKinematics->LL_UserData();
    if (ini)
    {
        if (ini->section_exist("immunities"))
        {
            LPCSTR imm_sect = ini->r_string("immunities", "immunities_sect");
            conditions().LoadImmunities(imm_sect, pSettings);
        }

        if (ini->line_exist("bone_protection", "bones_protection_sect"))
        {
            m_boneHitProtection = new SBoneProtections();
            m_boneHitProtection->reload(ini->r_string("bone_protection", "bones_protection_sect"), pKinematics);
        }
    }

    //вычислить иммунета в зависимости от ранга
    static float novice_rank_immunity = pSettings->r_float("ranks_properties", "immunities_novice_k");
    static float expirienced_rank_immunity = pSettings->r_float("ranks_properties", "immunities_experienced_k");

    static float novice_rank_visibility = pSettings->r_float("ranks_properties", "visibility_novice_k");
    static float expirienced_rank_visibility = pSettings->r_float("ranks_properties", "visibility_experienced_k");

    static float novice_rank_dispersion = pSettings->r_float("ranks_properties", "dispersion_novice_k");
    static float expirienced_rank_dispersion = pSettings->r_float("ranks_properties", "dispersion_experienced_k");

    CHARACTER_RANK_VALUE rank = Rank();
    clamp(rank, 0, 100);
    float rank_k = float(rank) / 100.f;
    m_fRankImmunity = novice_rank_immunity + (expirienced_rank_immunity - novice_rank_immunity) * rank_k;
    m_fRankVisibility = novice_rank_visibility + (expirienced_rank_visibility - novice_rank_visibility) * rank_k;
    m_fRankDisperison =
        expirienced_rank_dispersion + (novice_rank_dispersion - expirienced_rank_dispersion) * (1 - rank_k);

    if (!fis_zero(SpecificCharacter().panic_threshold()))
        m_panic_threshold = SpecificCharacter().panic_threshold();

    sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));

#ifdef _DEBUG
    if (ai().get_alife() && !Level().MapManager().HasMapLocation("debug_stalker", ID()))
    {
        CMapLocation* map_location = Level().MapManager().AddMapLocation("debug_stalker", ID());

        map_location->SetHint(cName());
    }
#endif // _DEBUG

    if (SpecificCharacter().terrain_sect().size())
    {
        movement().locations().Load(*SpecificCharacter().terrain_sect());
    }

    sight().update();
    Exec_Look(.001f);

    m_pPhysics_support->in_NetSpawn(e);

    return (TRUE);
}

void CAI_Stalker::net_Destroy()
{
    inherited::net_Destroy();
    CInventoryOwner::net_Destroy();
    m_pPhysics_support->in_NetDestroy();

    Device.remove_from_seq_parallel(fastdelegate::FastDelegate0<>(this, &CAI_Stalker::update_object_handler));

#ifdef DEBUG
    fastdelegate::FastDelegate0<> f = fastdelegate::FastDelegate0<>(this, &CAI_Stalker::update_object_handler);
    xr_vector<fastdelegate::FastDelegate0<>>::const_iterator I;
    I = std::find(Device.seqParallel.begin(), Device.seqParallel.end(), f);
    VERIFY(I == Device.seqParallel.end());
#endif // DEBUG

    xr_delete(m_ce_close);
    xr_delete(m_ce_far);
    xr_delete(m_ce_best);
    xr_delete(m_ce_angle);
    xr_delete(m_ce_safe);
    xr_delete(m_ce_ambush);
    xr_delete(m_boneHitProtection);
}

void CAI_Stalker::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    m_pPhysics_support->in_NetSave(P);
}

BOOL CAI_Stalker::net_SaveRelevant() { return (inherited::net_SaveRelevant() || BOOL(PPhysicsShell() != NULL)); }
void CAI_Stalker::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());

    // export last known packet
    R_ASSERT(!NET.empty());
    net_update& N = NET.back();
    //	P.w_float						(inventory().TotalWeight());
    //	P.w_u32							(m_dwMoney);

    P.w_float(GetfHealth());

    P.w_u32(N.dwTimeStamp);
    P.w_u8(0);
    P.w_vec3(N.p_pos);
    P.w_float /*w_angle8*/ (N.o_model);
    P.w_float /*w_angle8*/ (N.o_torso.yaw);
    P.w_float /*w_angle8*/ (N.o_torso.pitch);
    P.w_float /*w_angle8*/ (N.o_torso.roll);
    P.w_u8(u8(g_Team()));
    P.w_u8(u8(g_Squad()));
    P.w_u8(u8(g_Group()));

    float f1 = 0;
    GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    //	P.w						(&f1,						sizeof(f1));
    //	P.w						(&f1,						sizeof(f1));
    if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
    {
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
    }
    else
    {
        P.w(&f1, sizeof(f1));
        P.w(&f1, sizeof(f1));
    }

    P.w_stringZ(m_sStartDialog);
}

void CAI_Stalker::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());
    net_update N;

    u8 flags;

    P.r_float();
    set_money(P.r_u32(), false);

    float health;
    P.r_float(health);
    SetfHealth(health);
    //	fEntityHealth = health;

    P.r_u32(N.dwTimeStamp);
    P.r_u8(flags);
    P.r_vec3(N.p_pos);
    P.r_float /*r_angle8*/ (N.o_model);
    P.r_float /*r_angle8*/ (N.o_torso.yaw);
    P.r_float /*r_angle8*/ (N.o_torso.pitch);
    P.r_float /*r_angle8*/ (N.o_torso.roll);
    id_Team = P.r_u8();
    id_Squad = P.r_u8();
    id_Group = P.r_u8();

    GameGraph::_GRAPH_ID graph_vertex_id = movement().game_dest_vertex_id();
    P.r(&graph_vertex_id, sizeof(GameGraph::_GRAPH_ID));
    graph_vertex_id = ai_location().game_vertex_id();
    P.r(&graph_vertex_id, sizeof(GameGraph::_GRAPH_ID));

    if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp))
    {
        NET.push_back(N);
        NET_WasInterpolating = TRUE;
    }

    P.r_float();
    P.r_float();

    P.r_stringZ(m_sStartDialog);

    setVisible(TRUE);
    setEnabled(TRUE);
}

void CAI_Stalker::update_object_handler()
{
    if (!g_Alive())
        return;

    try
    {
        try
        {
            CObjectHandler::update();
        }
#ifdef DEBUG
        catch (luabind::cast_failed& message)
        {
            Msg("! Expression \"%s\" from luabind::object to %s", message.what(), message.info().name());
            throw;
        }
#endif
        catch (std::exception& message)
        {
            Msg("! Expression \"%s\"", message.what());
            throw;
        }
        catch (...)
        {
            throw;
        }
    }
    catch (...)
    {
        CObjectHandler::set_goal(eObjectActionIdle);
        CObjectHandler::update();
    }
}

void CAI_Stalker::create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation)
{
    inherited::create_anim_mov_ctrl(b, start_pose, local_animation);
}

void CAI_Stalker::destroy_anim_mov_ctrl()
{
    inherited::destroy_anim_mov_ctrl();

    if (!g_Alive())
        return;

    if (getDestroy())
        return;

    movement().m_head.current.yaw = movement().m_body.current.yaw;
    movement().m_head.current.pitch = movement().m_body.current.pitch;
    movement().m_head.target.yaw = movement().m_body.current.yaw;
    movement().m_head.target.pitch = movement().m_body.current.pitch;

    movement().cleanup_after_animation_selector();
    movement().update(0);
}

void CAI_Stalker::UpdateCL()
{
    START_PROFILE("stalker")
    START_PROFILE("stalker/client_update")
    VERIFY2(PPhysicsShell() || getEnabled(), *cName());

    if (g_Alive())
    {
        if (g_mt_config.test(mtObjectHandler) && CObjectHandler::planner().initialized())
        {
            fastdelegate::FastDelegate0<> f = fastdelegate::FastDelegate0<>(this, &CAI_Stalker::update_object_handler);
#ifdef DEBUG
            xr_vector<fastdelegate::FastDelegate0<>>::const_iterator I;
            I = std::find(Device.seqParallel.begin(), Device.seqParallel.end(), f);
            VERIFY(I == Device.seqParallel.end());
#endif
            Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CAI_Stalker::update_object_handler));
        }
        else
        {
            START_PROFILE("stalker/client_update/object_handler")
            update_object_handler();
            STOP_PROFILE
        }

        if ((movement().speed(character_physics_support()->movement()) > EPS_L) &&
            (eMovementTypeStand != movement().movement_type()) && (eMentalStateDanger == movement().mental_state()))
        {
            if ((eBodyStateStand == movement().body_state()) && (eMovementTypeRun == movement().movement_type()))
            {
                sound().play(eStalkerSoundRunningInDanger);
            }
            else
            {
                //				sound().play	(eStalkerSoundWalkingInDanger);
            }
        }
    }

    START_PROFILE("stalker/client_update/inherited")
    inherited::UpdateCL();
    STOP_PROFILE

    START_PROFILE("stalker/client_update/physics")
    m_pPhysics_support->in_UpdateCL();
    STOP_PROFILE

    if (g_Alive())
    {
        START_PROFILE("stalker/client_update/sight_manager")
        VERIFY(!m_pPhysicsShell);
        try
        {
            sight().update();
        }
        catch (...)
        {
            sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
            sight().update();
        }

        Exec_Look(client_update_fdelta());
        STOP_PROFILE

        START_PROFILE("stalker/client_update/step_manager")
        CStepManager::update(false);
        STOP_PROFILE

        START_PROFILE("stalker/client_update/weapon_shot_effector")
        if (weapon_shot_effector().IsActive())
            weapon_shot_effector().Update();
        STOP_PROFILE
    }
#ifdef DEBUG
    debug_text();
#endif
    STOP_PROFILE
    STOP_PROFILE
}

void CAI_Stalker::PHHit(SHit& H) { m_pPhysics_support->in_Hit(H, false); }
CPHDestroyable* CAI_Stalker::ph_destroyable() { return smart_cast<CPHDestroyable*>(character_physics_support()); }
#include "enemy_manager.h"

void CAI_Stalker::shedule_Update(u32 DT)
{
    START_PROFILE("stalker")
    START_PROFILE("stalker/schedule_update")
    VERIFY2(getEnabled() || PPhysicsShell(), *cName());

    if (!CObjectHandler::planner().initialized())
    {
        START_PROFILE("stalker/client_update/object_handler")
        update_object_handler();
        STOP_PROFILE
    }
    //	if (Position().distance_to(Level().CurrentEntity()->Position()) <= 50.f)
    //		Msg				("[%6d][SH][%s]",Device.dwTimeGlobal,*cName());
    // Queue shrink
    VERIFY(_valid(Position()));
    u32 dwTimeCL = Level().timeServer() - NET_Latency;
    VERIFY(!NET.empty());
    while ((NET.size() > 2) && (NET[1].dwTimeStamp < dwTimeCL))
        NET.pop_front();

    Fvector vNewPosition = Position();
    VERIFY(_valid(Position()));
    // *** general stuff
    float dt = float(DT) / 1000.f;

    if (g_Alive())
    {
        animation().play_delayed_callbacks();

#ifndef USE_SCHEDULER_IN_AGENT_MANAGER
        agent_manager().update();
#endif // USE_SCHEDULER_IN_AGENT_MANAGER

//		bool			check = !!memory().enemy().selected();
#if 0 // def DEBUG
		memory().visual().check_visibles();
#endif
        if (false && g_mt_config.test(mtAiVision))
            Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CCustomMonster::Exec_Visibility));
        else
        {
            START_PROFILE("stalker/schedule_update/vision")
            Exec_Visibility();
            STOP_PROFILE
        }

        START_PROFILE("stalker/schedule_update/memory")

        START_PROFILE("stalker/schedule_update/memory/process")
        process_enemies();
        STOP_PROFILE

        START_PROFILE("stalker/schedule_update/memory/update")
        memory().update(dt);
        STOP_PROFILE

        STOP_PROFILE
    }

    START_PROFILE("stalker/schedule_update/inherited")
    CEntityAlive::shedule_Update(DT);
    STOP_PROFILE

    if (Remote())
    {
    }
    else
    {
        // here is monster AI call
        VERIFY(_valid(Position()));
        m_fTimeUpdateDelta = dt;
        Level().AIStats.Think.Begin();
        if (GetScriptControl())
            ProcessScripts();
        else
#ifdef DEBUG
            if (Device.dwFrame > (spawn_time() + g_AI_inactive_time))
#endif
            Think();
        m_dwLastUpdateTime = Device.dwTimeGlobal;
        Level().AIStats.Think.End();
        VERIFY(_valid(Position()));

        // Look and action streams
        float temp = conditions().health();
        if (temp > 0)
        {
            START_PROFILE("stalker/schedule_update/feel_touch")
            Fvector C;
            float R;
            Center(C);
            R = Radius();
            feel_touch_update(C, R);
            STOP_PROFILE

            START_PROFILE("stalker/schedule_update/net_update")
            net_update uNext;
            uNext.dwTimeStamp = Level().timeServer();
            uNext.o_model = movement().m_body.current.yaw;
            uNext.o_torso = movement().m_head.current;
            uNext.p_pos = vNewPosition;
            uNext.fHealth = GetfHealth();
            NET.push_back(uNext);
            STOP_PROFILE
        }
        else
        {
            START_PROFILE("stalker/schedule_update/net_update")
            net_update uNext;
            uNext.dwTimeStamp = Level().timeServer();
            uNext.o_model = movement().m_body.current.yaw;
            uNext.o_torso = movement().m_head.current;
            uNext.p_pos = vNewPosition;
            uNext.fHealth = GetfHealth();
            NET.push_back(uNext);
            STOP_PROFILE
        }
    }
    VERIFY(_valid(Position()));

    START_PROFILE("stalker/schedule_update/inventory_owner")
    UpdateInventoryOwner(DT);
    STOP_PROFILE

    //#ifdef DEBUG
    //	if (psAI_Flags.test(aiALife)) {
    //		smart_cast<CSE_ALifeHumanStalker*>(ai().alife().objects().object(ID()))->check_inventory_consistency();
    //	}
    //#endif

    START_PROFILE("stalker/schedule_update/physics")
    VERIFY(_valid(Position()));
    m_pPhysics_support->in_shedule_Update(DT);
    VERIFY(_valid(Position()));
    STOP_PROFILE
    STOP_PROFILE
    STOP_PROFILE
}

float CAI_Stalker::Radius() const
{
    float R = inherited::Radius();
    CWeapon* W = smart_cast<CWeapon*>(inventory().ActiveItem());
    if (W)
        R += W->Radius();
    return R;
}

void CAI_Stalker::spawn_supplies()
{
    inherited::spawn_supplies();
    CObjectHandler::spawn_supplies();
}

void CAI_Stalker::Think()
{
    START_PROFILE("stalker/schedule_update/think")
    u32 update_delta = Device.dwTimeGlobal - m_dwLastUpdateTime;

    START_PROFILE("stalker/schedule_update/think/brain")
    //	try {
    //		try {
    brain().update(update_delta);
//		}
#ifdef DEBUG
//		catch (luabind::cast_failed &message) {
//			Msg						("! Expression \"%s\" from luabind::object to
//%s",message.what(),message.info().name());
// throw;
//		}
#endif
//		catch (std::exception &message) {
//			Msg						("! Expression \"%s\"",message.what());
//			throw;
//		}
//		catch (...) {
//			Msg						("! unknown exception occured");
//			throw;
//		}
//	}
//	catch(...) {
#ifdef DEBUG
//		Msg						("! Last action being executed : %s",brain().current_action().m_action_name);
#endif
    //		brain().setup			(this);
    //		brain().update			(update_delta);
    //	}
    STOP_PROFILE

    START_PROFILE("stalker/schedule_update/think/movement")
    if (!g_Alive())
        return;

    //	try {
    movement().update(update_delta);
//	}
#if 0 // def DEBUG
	catch (luabind::cast_failed &message) {
		Msg						("! Expression \"%s\" from luabind::object to %s",message.what(),message.info().name());
		movement().initialize	();
		movement().update		(update_delta);
		throw;
	}
	catch (std::exception &message) {
		Msg						("! Expression \"%s\"",message.what());
		movement().initialize	();
		movement().update		(update_delta);
		throw;
	}
	catch (...) {
		Msg						("! unknown exception occured");
		movement().initialize	();
		movement().update		(update_delta);
		throw;
	}
#endif // DEBUG

    STOP_PROFILE
    STOP_PROFILE
}

void CAI_Stalker::SelectAnimation(const Fvector& view, const Fvector& move, float speed)
{
    if (!Device.Paused())
        animation().update();
}

const SRotation CAI_Stalker::Orientation() const { return (movement().m_head.current); }
const MonsterSpace::SBoneRotation& CAI_Stalker::head_orientation() const { return (movement().head_orientation()); }
void CAI_Stalker::net_Relcase(IGameObject* O)
{
    inherited::net_Relcase(O);

    sight().remove_links(O);
    movement().remove_links(O);

    if (!g_Alive())
        return;

    agent_manager().remove_links(O);
    m_pPhysics_support->in_NetRelcase(O);
}

CMovementManager* CAI_Stalker::create_movement_manager()
{
    return (m_movement_manager = new stalker_movement_manager_smart_cover(this));
}

CSound_UserDataVisitor* CAI_Stalker::create_sound_visitor()
{
    return (m_sound_user_data_visitor = new CStalkerSoundDataVisitor(this));
}

CMemoryManager* CAI_Stalker::create_memory_manager() { return (new CMemoryManager(this, create_sound_visitor())); }
IFactoryObject* CAI_Stalker::_construct()
{
    m_pPhysics_support = new CCharacterPhysicsSupport(CCharacterPhysicsSupport::etStalker, this);
    CCustomMonster::_construct();
    CObjectHandler::_construct();
    CStepManager::_construct();

    m_actor_relation_flags.zero();
    m_animation_manager = new CStalkerAnimationManager(this);
    m_brain = new CStalkerPlanner();
    m_sight_manager = new CSightManager(this);
    m_weapon_shot_effector = new CWeaponShotEffector();

    return (this);
}

bool CAI_Stalker::use_center_to_aim() const { return (!wounded() && (movement().body_state() != eBodyStateCrouch)); }
void CAI_Stalker::UpdateCamera()
{
    float new_range = eye_range, new_fov = eye_fov;
    Fvector temp = eye_matrix.k;
    if (g_Alive())
    {
        update_range_fov(
            new_range, new_fov, memory().visual().current_state().m_max_view_distance * eye_range, eye_fov);
        if (weapon_shot_effector().IsActive())
            temp = weapon_shot_effector_direction(temp);
    }

    g_pGameLevel->Cameras().Update(eye_matrix.c, temp, eye_matrix.j, new_fov, .75f, new_range, 0);
}

bool CAI_Stalker::can_attach(const CInventoryItem* inventory_item) const
{
    if (already_dead())
        return (false);

    return (CObjectHandler::can_attach(inventory_item));
}

void CAI_Stalker::save(NET_Packet& packet)
{
    inherited::save(packet);
    CInventoryOwner::save(packet);
    brain().save(packet);
}

void CAI_Stalker::load(IReader& packet)
{
    inherited::load(packet);
    CInventoryOwner::load(packet);
    brain().load(packet);
}

void CAI_Stalker::load_critical_wound_bones()
{
    fill_bones_body_parts("head", critical_wound_type_head);
    fill_bones_body_parts("torso", critical_wound_type_torso);
    fill_bones_body_parts("hand_left", critical_wound_type_hand_left);
    fill_bones_body_parts("hand_right", critical_wound_type_hand_right);
    fill_bones_body_parts("leg_left", critical_wound_type_leg_left);
    fill_bones_body_parts("leg_right", critical_wound_type_leg_right);
}

void CAI_Stalker::fill_bones_body_parts(LPCSTR bone_id, const ECriticalWoundType& wound_type)
{
    LPCSTR body_parts_section_id = pSettings->r_string(cNameSect(), "body_parts_section_id");
    VERIFY(body_parts_section_id);

    LPCSTR body_part_section_id = pSettings->r_string(body_parts_section_id, bone_id);
    VERIFY(body_part_section_id);

    IKinematics* kinematics = smart_cast<IKinematics*>(Visual());
    VERIFY(kinematics);

    CInifile::Sect& body_part_section = pSettings->r_section(body_part_section_id);
    auto I = body_part_section.Data.cbegin();
    auto E = body_part_section.Data.cend();
    for (; I != E; ++I)
        m_bones_body_parts.insert(std::make_pair(kinematics->LL_BoneID((*I).first), u32(wound_type)));
}

void CAI_Stalker::on_before_change_team()
{
    m_registered_in_combat_on_migration = agent_manager().member().registered_in_combat(this);
}

void CAI_Stalker::on_after_change_team()
{
    if (!m_registered_in_combat_on_migration)
        return;

    agent_manager().member().register_in_combat(this);
}

float CAI_Stalker::shedule_Scale()
{
    if (!sniper_update_rate())
        return (inherited::shedule_Scale());

    return (0.f);
}

void CAI_Stalker::aim_bone_id(shared_str const& bone_id)
{
    //	IKinematics				*kinematics = smart_cast<IKinematics*>(Visual());
    //	VERIFY2					(kinematics->LL_BoneID(bone_id) != BI_NONE, make_string("Cannot find bone %s",bone_id));
    m_aim_bone_id = bone_id;
}

shared_str const& CAI_Stalker::aim_bone_id() const { return (m_aim_bone_id); }
void aim_target(shared_str const& aim_bone_id, Fvector& result, const CGameObject* object)
{
    IKinematics* kinematics = smart_cast<IKinematics*>(object->Visual());
    VERIFY(kinematics);

    u16 bone_id = kinematics->LL_BoneID(aim_bone_id);
    VERIFY2(bone_id != BI_NONE, make_string("Cannot find bone %s", bone_id));

    Fmatrix const& bone_matrix = kinematics->LL_GetTransform(bone_id);
    Fmatrix final;
    final.mul_43(object->XFORM(), bone_matrix);
    result = final.c;
}

void CAI_Stalker::aim_target(Fvector& result, const CGameObject* object)
{
    VERIFY(m_aim_bone_id.size());

    ::aim_target(m_aim_bone_id, result, object);
}

BOOL CAI_Stalker::AlwaysTheCrow()
{
    VERIFY(character_physics_support());
    return (character_physics_support()->is_interactive_motion());
}

smart_cover::cover const* CAI_Stalker::get_current_smart_cover()
{
    if (movement().current_params().cover_id() != movement().target_params().cover_id())
        return 0;

    return movement().current_params().cover();
}

smart_cover::loophole const* CAI_Stalker::get_current_loophole()
{
    if (movement().current_params().cover_id() != movement().target_params().cover_id())
        return 0;

    if (movement().current_params().cover_loophole_id() != movement().target_params().cover_loophole_id())
        return 0;

    return movement().current_params().cover_loophole();
}

bool CAI_Stalker::can_fire_right_now()
{
    if (!ready_to_kill())
        return (false);

    VERIFY(best_weapon());
    CWeapon& best_weapon = smart_cast<CWeapon&>(*this->best_weapon());
    return best_weapon.GetAmmoElapsed() > 0;
}

bool CAI_Stalker::unlimited_ammo() { return infinite_ammo() && CObjectHandler::planner().object().g_Alive(); }
