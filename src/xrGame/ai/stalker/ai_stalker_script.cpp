////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_script.cpp
//	Created 	: 29.09.2003
//  Modified 	: 29.09.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker script functions
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "stalker_decision_space.h"
#include "ai_stalker_space.h"
#include "script_game_object.h"
#include "stalker_planner.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CStalkerPlanner, (CScriptActionPlanner),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CStalkerPlanner, CScriptActionPlanner>("stalker_ids")
            .enum_("properties")
            [
                value("property_alive", StalkerDecisionSpace::eWorldPropertyAlive),
                value("property_dead", StalkerDecisionSpace::eWorldPropertyDead),
                value("property_already_dead", StalkerDecisionSpace::eWorldPropertyAlreadyDead),
                value("property_alife", StalkerDecisionSpace::eWorldPropertyALife),
                value("property_puzzle_solved", StalkerDecisionSpace::eWorldPropertyPuzzleSolved),
                value("property_smart_terrain_task", StalkerDecisionSpace::eWorldPropertySmartTerrainTask),
                value("property_items", StalkerDecisionSpace::eWorldPropertyItems),
                value("property_enemy", StalkerDecisionSpace::eWorldPropertyEnemy),
                value("property_danger", StalkerDecisionSpace::eWorldPropertyDanger),
                value("property_item_to_kill", StalkerDecisionSpace::eWorldPropertyItemToKill),
                value("property_found_item_to_kill", StalkerDecisionSpace::eWorldPropertyFoundItemToKill),
                value("property_item_can_kill", StalkerDecisionSpace::eWorldPropertyItemCanKill),
                value("property_found_ammo", StalkerDecisionSpace::eWorldPropertyFoundAmmo),
                value("property_ready_to_kill", StalkerDecisionSpace::eWorldPropertyReadyToKill),
                value("property_ready_to_detour", StalkerDecisionSpace::eWorldPropertyReadyToDetour),
                value("property_see_enemy", StalkerDecisionSpace::eWorldPropertySeeEnemy),
                value("property_panic", StalkerDecisionSpace::eWorldPropertyPanic),
                value("property_in_cover", StalkerDecisionSpace::eWorldPropertyInCover),
                value("property_looked_out", StalkerDecisionSpace::eWorldPropertyLookedOut),
                value("property_position_holded", StalkerDecisionSpace::eWorldPropertyPositionHolded),
                value("property_enemy_detoured", StalkerDecisionSpace::eWorldPropertyEnemyDetoured),
                value("property_use_suddenness", StalkerDecisionSpace::eWorldPropertyUseSuddenness),
                value("property_use_crouch_to_look_out", StalkerDecisionSpace::eWorldPropertyUseCrouchToLookOut),
                value("property_critically_wounded", StalkerDecisionSpace::eWorldPropertyCriticallyWounded),
                value("property_enemy_critically_wounded",StalkerDecisionSpace::eWorldPropertyEnemyCriticallyWounded),

                value("property_danger_unknown", StalkerDecisionSpace::eWorldPropertyDangerUnknown),
                value("property_danger_in_direction", StalkerDecisionSpace::eWorldPropertyDangerInDirection),
                value("property_danger_grenade", StalkerDecisionSpace::eWorldPropertyDangerGrenade),
                value("property_danger_by_sound", StalkerDecisionSpace::eWorldPropertyDangerBySound),

                value("property_cover_actual", StalkerDecisionSpace::eWorldPropertyCoverActual),
                value("property_cover_reached", StalkerDecisionSpace::eWorldPropertyCoverReached),
                value("property_looked_around", StalkerDecisionSpace::eWorldPropertyLookedAround),
                value("property_grenade_exploded", StalkerDecisionSpace::eWorldPropertyGrenadeExploded),

                value("property_anomaly", StalkerDecisionSpace::eWorldPropertyAnomaly),
                value("property_inside_anomaly", StalkerDecisionSpace::eWorldPropertyInsideAnomaly),
                value("property_pure_enemy", StalkerDecisionSpace::eWorldPropertyPureEnemy),
                value("property_script", StalkerDecisionSpace::eWorldPropertyScript)
            ]

            .enum_("action")
            [
                value("action_dead", StalkerDecisionSpace::eWorldOperatorDead),
                value("action_dying", StalkerDecisionSpace::eWorldOperatorDying),
                value("action_gather_items", StalkerDecisionSpace::eWorldOperatorGatherItems),
                value("action_no_alife", StalkerDecisionSpace::eWorldOperatorALifeEmulation),
                value("action_smart_terrain_task", StalkerDecisionSpace::eWorldOperatorSmartTerrainTask),
                value("action_solve_zone_puzzle", StalkerDecisionSpace::eWorldOperatorSolveZonePuzzle),
                value("action_reach_task_location", StalkerDecisionSpace::eWorldOperatorReachTaskLocation),
                value("action_accomplish_task", StalkerDecisionSpace::eWorldOperatorAccomplishTask),
                value("action_reach_customer_location", StalkerDecisionSpace::eWorldOperatorReachCustomerLocation),
                value("action_communicate_with_customer",StalkerDecisionSpace::eWorldOperatorCommunicateWithCustomer),
                value("get_out_of_anomaly", StalkerDecisionSpace::eWorldOperatorGetOutOfAnomaly),
                value("detect_anomaly", StalkerDecisionSpace::eWorldOperatorDetectAnomaly),
                value("action_get_item_to_kill", StalkerDecisionSpace::eWorldOperatorGetItemToKill),
                value("action_find_item_to_kill", StalkerDecisionSpace::eWorldOperatorFindItemToKill),
                value("action_make_item_killing", StalkerDecisionSpace::eWorldOperatorMakeItemKilling),
                value("action_find_ammo", StalkerDecisionSpace::eWorldOperatorFindAmmo),
                value("action_aim_enemy", StalkerDecisionSpace::eWorldOperatorAimEnemy),
                value("action_get_ready_to_kill", StalkerDecisionSpace::eWorldOperatorGetReadyToKill),
                value("action_kill_enemy", StalkerDecisionSpace::eWorldOperatorKillEnemy),
                value("action_retreat_from_enemy", StalkerDecisionSpace::eWorldOperatorRetreatFromEnemy),
                value("action_take_cover", StalkerDecisionSpace::eWorldOperatorTakeCover),
                value("action_look_out", StalkerDecisionSpace::eWorldOperatorLookOut),
                value("action_hold_position", StalkerDecisionSpace::eWorldOperatorHoldPosition),
                value("action_get_distance", StalkerDecisionSpace::eWorldOperatorGetDistance),
                value("action_detour_enemy", StalkerDecisionSpace::eWorldOperatorDetourEnemy),
                value("action_search_enemy", StalkerDecisionSpace::eWorldOperatorSearchEnemy),
                value("action_sudden_attack", StalkerDecisionSpace::eWorldOperatorSuddenAttack),
                value("action_kill_enemy_if_not_visible", StalkerDecisionSpace::eWorldOperatorKillEnemyIfNotVisible),
                value("action_reach_wounded_enemy", StalkerDecisionSpace::eWorldOperatorReachWoundedEnemy),
                value("action_prepare_wounded_enemy", StalkerDecisionSpace::eWorldOperatorPrepareWoundedEnemy),
                value("action_kill_wounded_enemy", StalkerDecisionSpace::eWorldOperatorKillWoundedEnemy),
                value("action_kill_if_player_on_the_path",StalkerDecisionSpace::eWorldOperatorKillEnemyIfPlayerOnThePath),
                value("action_critically_wounded", StalkerDecisionSpace::eWorldOperatorCriticallyWounded),
                value("action_kill_if_enemy_critically_wounded",StalkerDecisionSpace::eWorldOperatorKillEnemyIfCriticallyWounded),

                value("action_danger_unknown_planner", StalkerDecisionSpace::eWorldOperatorDangerUnknownPlanner),
                value("action_danger_in_direction_planner",StalkerDecisionSpace::eWorldOperatorDangerInDirectionPlanner),
                value("action_danger_grenade_planner", StalkerDecisionSpace::eWorldOperatorDangerGrenadePlanner),
                value("action_danger_by_sound_planner", StalkerDecisionSpace::eWorldOperatorDangerBySoundPlanner),

                value("action_danger_unknown_take_cover", StalkerDecisionSpace::eWorldOperatorDangerUnknownTakeCover),
                value("action_danger_unknown_look_around", StalkerDecisionSpace::eWorldOperatorDangerUnknownLookAround),
                value("action_danger_unknown_search", StalkerDecisionSpace::eWorldOperatorDangerUnknownSearchEnemy),

                value("action_danger_in_direction_take_cover", StalkerDecisionSpace::eWorldOperatorDangerInDirectionTakeCover),
                value("action_danger_in_direction_look_out", StalkerDecisionSpace::eWorldOperatorDangerInDirectionLookOut),
                value("action_danger_in_direction_hold_position", StalkerDecisionSpace::eWorldOperatorDangerInDirectionHoldPosition),
                value("action_danger_in_direction_detour", StalkerDecisionSpace::eWorldOperatorDangerInDirectionDetourEnemy),
                value("action_danger_in_direction_search", StalkerDecisionSpace::eWorldOperatorDangerInDirectionSearchEnemy),

                value("action_danger_grenade_take_cover", StalkerDecisionSpace::eWorldOperatorDangerGrenadeTakeCover),
                value("action_danger_grenade_wait_for_explosion", StalkerDecisionSpace::eWorldOperatorDangerGrenadeWaitForExplosion),
                value("action_danger_grenade_take_cover_after_explosion", StalkerDecisionSpace::eWorldOperatorDangerGrenadeTakeCoverAfterExplosion),
                value("action_danger_grenade_look_around", StalkerDecisionSpace::eWorldOperatorDangerGrenadeLookAround),
                value("action_danger_grenade_search", StalkerDecisionSpace::eWorldOperatorDangerGrenadeSearch),

                value("action_death_planner", StalkerDecisionSpace::eWorldOperatorDeathPlanner),
                value("action_alife_planner", StalkerDecisionSpace::eWorldOperatorALifePlanner),
                value("action_combat_planner", StalkerDecisionSpace::eWorldOperatorCombatPlanner),
                value("action_anomaly_planner", StalkerDecisionSpace::eWorldOperatorAnomalyPlanner),
                value("action_danger_planner", StalkerDecisionSpace::eWorldOperatorDangerPlanner),
                value("action_post_combat_wait", StalkerDecisionSpace::eWorldOperatorPostCombatWait),
                value("action_script", StalkerDecisionSpace::eWorldOperatorScript)
            ]

            .enum_("sounds")
            [
                value("sound_die", StalkerSpace::eStalkerSoundDie),
                value("sound_die_in_anomaly", StalkerSpace::eStalkerSoundDieInAnomaly),
                value("sound_injuring", StalkerSpace::eStalkerSoundInjuring),
                value("sound_humming", StalkerSpace::eStalkerSoundHumming),
                value("sound_alarm", StalkerSpace::eStalkerSoundAlarm),
                value("sound_attack_no_allies", StalkerSpace::eStalkerSoundAttackNoAllies),
                value("sound_attack_allies_single_enemy", StalkerSpace::eStalkerSoundAttackAlliesSingleEnemy),
                value("sound_attack_allies_several_enemies", StalkerSpace::eStalkerSoundAttackAlliesSeveralEnemies),
                value("sound_backup", StalkerSpace::eStalkerSoundBackup),
                value("sound_detour", StalkerSpace::eStalkerSoundDetour),
                value("sound_search1_no_allies", StalkerSpace::eStalkerSoundSearch1NoAllies),
                value("sound_search1_with_allies", StalkerSpace::eStalkerSoundSearch1WithAllies),
                value("sound_enemy_lost_no_allies", StalkerSpace::eStalkerSoundEnemyLostNoAllies),
                value("sound_enemy_lost_with_allies", StalkerSpace::eStalkerSoundEnemyLostWithAllies),
                value("sound_injuring_by_friend", StalkerSpace::eStalkerSoundInjuringByFriend),
                value("sound_panic_human", StalkerSpace::eStalkerSoundPanicHuman),
                value("sound_panic_monster", StalkerSpace::eStalkerSoundPanicMonster),
                value("sound_tolls", StalkerSpace::eStalkerSoundTolls),
                value("sound_wounded", StalkerSpace::eStalkerSoundWounded),
                value("sound_grenade_alarm", StalkerSpace::eStalkerSoundGrenadeAlarm),
                value("sound_friendly_grenade_alarm", StalkerSpace::eStalkerSoundFriendlyGrenadeAlarm),
                value("sound_need_backup", StalkerSpace::eStalkerSoundNeedBackup),

                value("sound_running_in_danger", StalkerSpace::eStalkerSoundRunningInDanger),
                //value("sound_walking_in_danger", StalkerSpace::eStalkerSoundWalkingInDanger),
                value("sound_kill_wounded", StalkerSpace::eStalkerSoundKillWounded),
                value("sound_enemy_critically_wounded", StalkerSpace::eStalkerSoundEnemyCriticallyWounded),
                value("sound_enemy_killed_or_wounded", StalkerSpace::eStalkerSoundMaskEnemyKilledOrWounded),

                value("sound_script", StalkerSpace::eStalkerSoundScript)
            ]
    ];
});

SCRIPT_EXPORT(CAI_Stalker, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Stalker, CGameObject>("CAI_Stalker")
            .def(constructor<>())
    ];
});
