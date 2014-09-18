////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_script.cpp
//	Created 	: 29.09.2003
//  Modified 	: 29.09.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker script functions
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "../../stalker_decision_space.h"
#include "ai_stalker_space.h"
#include "../../script_game_object.h"
#include "../../stalker_planner.h"

using namespace luabind;

#pragma optimize("s",on)
void CAI_Stalker::script_register(lua_State *L)
{
	module(L)
	[
		class_<CStalkerPlanner>("stalker_ids")
			.enum_("properties")
			[
				luabind::value("property_alive",							StalkerDecisionSpace::eWorldPropertyAlive),
				luabind::value("property_dead",								StalkerDecisionSpace::eWorldPropertyDead),
				luabind::value("property_already_dead",						StalkerDecisionSpace::eWorldPropertyAlreadyDead),
				luabind::value("property_alife",							StalkerDecisionSpace::eWorldPropertyALife),
				luabind::value("property_puzzle_solved",					StalkerDecisionSpace::eWorldPropertyPuzzleSolved),
				luabind::value("property_smart_terrain_task",				StalkerDecisionSpace::eWorldPropertySmartTerrainTask),
				luabind::value("property_items",							StalkerDecisionSpace::eWorldPropertyItems),
				luabind::value("property_enemy",							StalkerDecisionSpace::eWorldPropertyEnemy),
				luabind::value("property_danger",							StalkerDecisionSpace::eWorldPropertyDanger),
				luabind::value("property_item_to_kill",						StalkerDecisionSpace::eWorldPropertyItemToKill),
				luabind::value("property_found_item_to_kill",				StalkerDecisionSpace::eWorldPropertyFoundItemToKill),
				luabind::value("property_item_can_kill",					StalkerDecisionSpace::eWorldPropertyItemCanKill),
				luabind::value("property_found_ammo",						StalkerDecisionSpace::eWorldPropertyFoundAmmo),
				luabind::value("property_ready_to_kill",					StalkerDecisionSpace::eWorldPropertyReadyToKill),
				luabind::value("property_ready_to_detour",					StalkerDecisionSpace::eWorldPropertyReadyToDetour),
				luabind::value("property_see_enemy",						StalkerDecisionSpace::eWorldPropertySeeEnemy),
				luabind::value("property_panic",							StalkerDecisionSpace::eWorldPropertyPanic),
				luabind::value("property_in_cover",							StalkerDecisionSpace::eWorldPropertyInCover),
				luabind::value("property_looked_out",						StalkerDecisionSpace::eWorldPropertyLookedOut),
				luabind::value("property_position_holded",					StalkerDecisionSpace::eWorldPropertyPositionHolded),
				luabind::value("property_enemy_detoured",					StalkerDecisionSpace::eWorldPropertyEnemyDetoured),
				luabind::value("property_use_suddenness",					StalkerDecisionSpace::eWorldPropertyUseSuddenness),
				luabind::value("property_use_crouch_to_look_out",			StalkerDecisionSpace::eWorldPropertyUseCrouchToLookOut),
				luabind::value("property_critically_wounded",				StalkerDecisionSpace::eWorldPropertyCriticallyWounded),
				luabind::value("property_enemy_critically_wounded",			StalkerDecisionSpace::eWorldPropertyEnemyCriticallyWounded),
				
				luabind::value("property_danger_unknown",					StalkerDecisionSpace::eWorldPropertyDangerUnknown),
				luabind::value("property_danger_in_direction",				StalkerDecisionSpace::eWorldPropertyDangerInDirection),
				luabind::value("property_danger_grenade",					StalkerDecisionSpace::eWorldPropertyDangerGrenade),
				luabind::value("property_danger_by_sound",					StalkerDecisionSpace::eWorldPropertyDangerBySound),

				luabind::value("property_cover_actual",						StalkerDecisionSpace::eWorldPropertyCoverActual),
				luabind::value("property_cover_reached",					StalkerDecisionSpace::eWorldPropertyCoverReached),
				luabind::value("property_looked_around",					StalkerDecisionSpace::eWorldPropertyLookedAround),
				luabind::value("property_grenade_exploded",					StalkerDecisionSpace::eWorldPropertyGrenadeExploded),

				luabind::value("property_anomaly",							StalkerDecisionSpace::eWorldPropertyAnomaly),
				luabind::value("property_inside_anomaly",					StalkerDecisionSpace::eWorldPropertyInsideAnomaly),
				luabind::value("property_pure_enemy",						StalkerDecisionSpace::eWorldPropertyPureEnemy),
				luabind::value("property_script",							StalkerDecisionSpace::eWorldPropertyScript)
			]
			
			.enum_("action")
			[
				luabind::value("action_dead",								StalkerDecisionSpace::eWorldOperatorDead),
				luabind::value("action_dying",								StalkerDecisionSpace::eWorldOperatorDying),
				luabind::value("action_gather_items",						StalkerDecisionSpace::eWorldOperatorGatherItems),
				luabind::value("action_no_alife",							StalkerDecisionSpace::eWorldOperatorALifeEmulation),
				luabind::value("action_smart_terrain_task",					StalkerDecisionSpace::eWorldOperatorSmartTerrainTask),
				luabind::value("action_solve_zone_puzzle",					StalkerDecisionSpace::eWorldOperatorSolveZonePuzzle),
				luabind::value("action_reach_task_location",				StalkerDecisionSpace::eWorldOperatorReachTaskLocation),
				luabind::value("action_accomplish_task",					StalkerDecisionSpace::eWorldOperatorAccomplishTask),
				luabind::value("action_reach_customer_location",			StalkerDecisionSpace::eWorldOperatorReachCustomerLocation),
				luabind::value("action_communicate_with_customer",			StalkerDecisionSpace::eWorldOperatorCommunicateWithCustomer),
				luabind::value("get_out_of_anomaly",						StalkerDecisionSpace::eWorldOperatorGetOutOfAnomaly),
				luabind::value("detect_anomaly",							StalkerDecisionSpace::eWorldOperatorDetectAnomaly),
				luabind::value("action_get_item_to_kill",					StalkerDecisionSpace::eWorldOperatorGetItemToKill),
				luabind::value("action_find_item_to_kill",					StalkerDecisionSpace::eWorldOperatorFindItemToKill),
				luabind::value("action_make_item_killing",					StalkerDecisionSpace::eWorldOperatorMakeItemKilling),
				luabind::value("action_find_ammo",							StalkerDecisionSpace::eWorldOperatorFindAmmo),
				luabind::value("action_aim_enemy",							StalkerDecisionSpace::eWorldOperatorAimEnemy),
				luabind::value("action_get_ready_to_kill",					StalkerDecisionSpace::eWorldOperatorGetReadyToKill),
				luabind::value("action_kill_enemy",							StalkerDecisionSpace::eWorldOperatorKillEnemy),
				luabind::value("action_retreat_from_enemy",					StalkerDecisionSpace::eWorldOperatorRetreatFromEnemy),
				luabind::value("action_take_cover",							StalkerDecisionSpace::eWorldOperatorTakeCover),
				luabind::value("action_look_out",							StalkerDecisionSpace::eWorldOperatorLookOut),
				luabind::value("action_hold_position",						StalkerDecisionSpace::eWorldOperatorHoldPosition),
				luabind::value("action_get_distance",						StalkerDecisionSpace::eWorldOperatorGetDistance),
				luabind::value("action_detour_enemy",						StalkerDecisionSpace::eWorldOperatorDetourEnemy),
				luabind::value("action_search_enemy",						StalkerDecisionSpace::eWorldOperatorSearchEnemy),
				luabind::value("action_sudden_attack",						StalkerDecisionSpace::eWorldOperatorSuddenAttack),
				luabind::value("action_kill_enemy_if_not_visible",			StalkerDecisionSpace::eWorldOperatorKillEnemyIfNotVisible),
				luabind::value("action_reach_wounded_enemy",				StalkerDecisionSpace::eWorldOperatorReachWoundedEnemy),
				luabind::value("action_prepare_wounded_enemy",				StalkerDecisionSpace::eWorldOperatorPrepareWoundedEnemy),
				luabind::value("action_kill_wounded_enemy",					StalkerDecisionSpace::eWorldOperatorKillWoundedEnemy),
				luabind::value("action_kill_if_player_on_the_path",			StalkerDecisionSpace::eWorldOperatorKillEnemyIfPlayerOnThePath),
				luabind::value("action_critically_wounded",					StalkerDecisionSpace::eWorldOperatorCriticallyWounded),
				luabind::value("action_kill_if_enemy_critically_wounded",	StalkerDecisionSpace::eWorldOperatorKillEnemyIfCriticallyWounded),

				luabind::value("action_danger_unknown_planner",				StalkerDecisionSpace::eWorldOperatorDangerUnknownPlanner),
				luabind::value("action_danger_in_direction_planner",		StalkerDecisionSpace::eWorldOperatorDangerInDirectionPlanner),
				luabind::value("action_danger_grenade_planner",				StalkerDecisionSpace::eWorldOperatorDangerGrenadePlanner),
				luabind::value("action_danger_by_sound_planner",			StalkerDecisionSpace::eWorldOperatorDangerBySoundPlanner),

				luabind::value("action_danger_unknown_take_cover",			StalkerDecisionSpace::eWorldOperatorDangerUnknownTakeCover),
				luabind::value("action_danger_unknown_look_around",			StalkerDecisionSpace::eWorldOperatorDangerUnknownLookAround),
				luabind::value("action_danger_unknown_search",				StalkerDecisionSpace::eWorldOperatorDangerUnknownSearchEnemy),

				luabind::value("action_danger_in_direction_take_cover",		StalkerDecisionSpace::eWorldOperatorDangerInDirectionTakeCover),
				luabind::value("action_danger_in_direction_look_out",		StalkerDecisionSpace::eWorldOperatorDangerInDirectionLookOut),
				luabind::value("action_danger_in_direction_hold_position",	StalkerDecisionSpace::eWorldOperatorDangerInDirectionHoldPosition),
				luabind::value("action_danger_in_direction_detour",			StalkerDecisionSpace::eWorldOperatorDangerInDirectionDetourEnemy),
				luabind::value("action_danger_in_direction_search",			StalkerDecisionSpace::eWorldOperatorDangerInDirectionSearchEnemy),

				luabind::value("action_danger_grenade_take_cover",			StalkerDecisionSpace::eWorldOperatorDangerGrenadeTakeCover),
				luabind::value("action_danger_grenade_wait_for_explosion",	StalkerDecisionSpace::eWorldOperatorDangerGrenadeWaitForExplosion),
				luabind::value("action_danger_grenade_take_cover_after_explosion",	StalkerDecisionSpace::eWorldOperatorDangerGrenadeTakeCoverAfterExplosion),
				luabind::value("action_danger_grenade_look_around",			StalkerDecisionSpace::eWorldOperatorDangerGrenadeLookAround),
				luabind::value("action_danger_grenade_search",				StalkerDecisionSpace::eWorldOperatorDangerGrenadeSearch),

				luabind::value("action_death_planner",						StalkerDecisionSpace::eWorldOperatorDeathPlanner),
				luabind::value("action_alife_planner",						StalkerDecisionSpace::eWorldOperatorALifePlanner),
				luabind::value("action_combat_planner",						StalkerDecisionSpace::eWorldOperatorCombatPlanner),
				luabind::value("action_anomaly_planner",					StalkerDecisionSpace::eWorldOperatorAnomalyPlanner),
				luabind::value("action_danger_planner",						StalkerDecisionSpace::eWorldOperatorDangerPlanner),
				luabind::value("action_post_combat_wait",					StalkerDecisionSpace::eWorldOperatorPostCombatWait),
				luabind::value("action_script",								StalkerDecisionSpace::eWorldOperatorScript)
			]

			.enum_("sounds")
			[
				luabind::value("sound_die",									StalkerSpace::eStalkerSoundDie),
				luabind::value("sound_die_in_anomaly",						StalkerSpace::eStalkerSoundDieInAnomaly),
				luabind::value("sound_injuring",							StalkerSpace::eStalkerSoundInjuring),
				luabind::value("sound_humming",								StalkerSpace::eStalkerSoundHumming),
				luabind::value("sound_alarm",								StalkerSpace::eStalkerSoundAlarm),
				luabind::value("sound_attack_no_allies",					StalkerSpace::eStalkerSoundAttackNoAllies),
				luabind::value("sound_attack_allies_single_enemy",			StalkerSpace::eStalkerSoundAttackAlliesSingleEnemy),
				luabind::value("sound_attack_allies_several_enemies",		StalkerSpace::eStalkerSoundAttackAlliesSeveralEnemies),
				luabind::value("sound_backup",								StalkerSpace::eStalkerSoundBackup),
				luabind::value("sound_detour",								StalkerSpace::eStalkerSoundDetour),
				luabind::value("sound_search1_no_allies",					StalkerSpace::eStalkerSoundSearch1NoAllies),
				luabind::value("sound_search1_with_allies",					StalkerSpace::eStalkerSoundSearch1WithAllies),
				luabind::value("sound_enemy_lost_no_allies",				StalkerSpace::eStalkerSoundEnemyLostNoAllies),
				luabind::value("sound_enemy_lost_with_allies",				StalkerSpace::eStalkerSoundEnemyLostWithAllies),
				luabind::value("sound_injuring_by_friend",					StalkerSpace::eStalkerSoundInjuringByFriend),
				luabind::value("sound_panic_human",							StalkerSpace::eStalkerSoundPanicHuman),
				luabind::value("sound_panic_monster",						StalkerSpace::eStalkerSoundPanicMonster),
				luabind::value("sound_tolls",								StalkerSpace::eStalkerSoundTolls),
				luabind::value("sound_wounded",								StalkerSpace::eStalkerSoundWounded),
				luabind::value("sound_grenade_alarm",						StalkerSpace::eStalkerSoundGrenadeAlarm),
				luabind::value("sound_friendly_grenade_alarm",				StalkerSpace::eStalkerSoundFriendlyGrenadeAlarm),
				luabind::value("sound_need_backup",							StalkerSpace::eStalkerSoundNeedBackup),

				luabind::value("sound_running_in_danger",					StalkerSpace::eStalkerSoundRunningInDanger),
//				luabind::value("sound_walking_in_danger",					StalkerSpace::eStalkerSoundWalkingInDanger),
				luabind::value("sound_kill_wounded",						StalkerSpace::eStalkerSoundKillWounded),
				luabind::value("sound_enemy_critically_wounded",			StalkerSpace::eStalkerSoundEnemyCriticallyWounded),
				luabind::value("sound_enemy_killed_or_wounded",				StalkerSpace::eStalkerSoundMaskEnemyKilledOrWounded),

				luabind::value("sound_script",								StalkerSpace::eStalkerSoundScript)
			],
		
		class_<CAI_Stalker,CGameObject>("CAI_Stalker")
			.def(constructor<>())
	];
}
