#include "StdAfx.h"

#ifdef DEBUG
namespace debug
{
class text_tree;
}

class CBaseMonster
{
public:
    virtual void add_debug_info(debug::text_tree& root_s);
};
#endif
#include "xrGame/ai/monsters/state.h"

xr_string make_xrstr(EMonsterState state) noexcept
{
    switch (state)
    {
    case eGlobalState: return "eGlobalState";
    case eStateRest: return "Rest";
    case eStateRest_WalkGraphPoint: return "Rest_WalkGraphPoint";
    case eStateRest_Idle: return "Rest_Idle";
    case eStateRest_Fun: return "Rest_Fun";
    case eStateRest_Sleep: return "Rest_Sleep";
    case eStateRest_MoveToHomePoint: return "Rest_MoveToHomePoint";
    case eStateRest_WalkToCover: return "Rest_WalkToCover";
    case eStateRest_LookOpenPlace: return "Rest_LookOpenPlace";
    case eStateEat: return "Eat";
    case eStateEat_CorpseApproachRun: return "Eat_CorpseApproachRun";
    case eStateEat_CorpseApproachWalk: return "Eat_CorpseApproachWalk";
    case eStateEat_CheckCorpse: return "Eat_CheckCorpse";
    case eStateEat_Eat: return "Eat_Eat";
    case eStateEat_WalkAway: return "Eat_WalkAway";
    case eStateEat_Rest: return "Eat_Rest";
    case eStateEat_Drag: return "Eat_Drag";
    case eStateAttack: return "Attack";
    case eStateAttack_Run: return "Attack_Run";
    case eStateAttack_Melee: return "Attack_Melee";
    case eStateAttack_RunAttack: return "Attack_RunAttack";
    case eStateAttack_RunAway: return "Attack_RunAway";
    case eStateAttack_FindEnemy: return "Attack_FindEnemy";
    case eStateAttack_Steal: return "Attack_Steal";
    case eStateAttack_AttackHidden: return "Attack_AttackHidden";
    case eStateAttack_Hide: return "Attack_Hide";
    case eStateAttack_HideInCover: return "Attack_HideInCover";
    case eStateAttack_MoveOut: return "Attack_MoveOut";
    case eStateAttack_CampInCover: return "Attack_CampInCover";
    case eStateAttack_ControlFire: return "Attack_ControlFire";
    case eStateAttack_ControlTube: return "Attack_ControlTube";
    case eStateAttack_HideInCoverLite: return "Attack_HideInCoverLite";
    case eStateAttackCamp: return "AttackCamp";
    case eStateAttackCamp_Hide: return "AttackCamp_Hide";
    case eStateAttackCamp_Camp: return "AttackCamp_Camp";
    case eStateAttackCamp_StealOut: return "AttackCamp_StealOut";
    case eStateAttack_Psy: return "Attack_Psy";
    case eStateAttack_MoveToHomePoint: return "Attack_MoveToHomePoint";
    case eStateAttack_HomePoint_Hide: return "Attack_HomePoint_Hide";
    case eStateAttack_HomePoint_Camp: return "Attack_HomePoint_Camp";
    case eStateAttack_HomePoint_LookOpenPlace: return "Attack_HomePoint_LookOpenPlace";
    case eStatePanic: return "Panic";
    case eStatePanic_Run: return "Panic_Run";
    case eStatePanic_FaceUnprotectedArea: return "Panic_FaceUnprotectedArea";
    case eStatePanic_MoveToHomePoint: return "Panic_MoveToHomePoint";
    case eStatePanic_HomePoint_Hide: return "Panic_HomePoint_Hide";
    case eStatePanic_HomePoint_LookOpenPlace: return "Panic_HomePoint_LookOpenPlace";
    case eStatePanic_HomePoint_Camp: return "Panic_HomePoint_Camp";
    case eStateHitted: return "Hitted";
    case eStateHitted_Hide: return "Hitted_Hide";
    case eStateHitted_MoveOut: return "Hitted_MoveOut";
    case eStateHitted_Home: return "Hitted_Home";
    case eStateHearDangerousSound: return "HearDangerousSound";
    case eStateHearDangerousSound_Hide: return "HearDangerousSound_Hide";
    case eStateHearDangerousSound_FaceOpenPlace: return "HearDangerousSound_FaceOpenPlace";
    case eStateHearDangerousSound_StandScared: return "HearDangerousSound_StandScared";
    case eStateHearDangerousSound_Home: return "HearDangerousSound_Home";
    case eStateHearInterestingSound: return "HearInterestingSound";
    case eStateHearInterestingSound_MoveToDest: return "HearInterestingSound_MoveToDest";
    case eStateHearInterestingSound_LookAround: return "HearInterestingSound_LookAround";
    case eStateHearHelpSound: return "HearHelpSound";
    case eStateHearHelpSound_MoveToDest: return "HearHelpSound_MoveToDest";
    case eStateHearHelpSound_LookAround: return "HearHelpSound_LookAround";
    case eStateControlled: return "Controlled";
    case eStateControlled_Follow: return "Controlled_Follow";
    case eStateControlled_Attack: return "Controlled_Attack";
    case eStateControlled_Follow_Wait: return "Controlled_Follow_Wait";
    case eStateControlled_Follow_WalkToObject: return "Controlled_Follow_WalkToObject";
    case eStateThreaten: return "Threaten";
    case eStateFindEnemy: return "FindEnemy";
    case eStateFindEnemy_Run: return "FindEnemy_Run";
    case eStateFindEnemy_LookAround: return "FindEnemy_LookAround";
    case eStateFindEnemy_Angry: return "FindEnemy_Angry";
    case eStateFindEnemy_WalkAround: return "FindEnemy_WalkAround";
    case eStateFindEnemy_LookAround_MoveToPoint: return "FindEnemy_LookAround_MoveToPoint";
    case eStateFindEnemy_LookAround_LookAround: return "FindEnemy_LookAround_LookAround";
    case eStateFindEnemy_LookAround_TurnToPoint: return "FindEnemy_LookAround_TurnToPoint";
    case eStateSquad: return "Squad";
    case eStateSquad_Rest: return "Squad_Rest";
    case eStateSquad_RestFollow: return "Squad_RestFollow";
    case eStateSquad_Rest_Idle: return "Squad_Rest_Idle";
    case eStateSquad_Rest_WalkAroundLeader: return "Squad_Rest_WalkAroundLeader";
    case eStateSquad_RestFollow_Idle: return "Squad_RestFollow_Idle";
    case eStateSquad_RestFollow_WalkToPoint: return "Squad_RestFollow_WalkToPoint";
    case eStateCustom: return "Custom";
    case eStateBurerScanning: return "BurerScanning";
    case eStateCustomMoveToRestrictor: return "CustomMoveToRestrictor";
    case eStateSmartTerrainTask: return "SmartTerrainTask";
    case eStateSmartTerrainTaskGamePathWalk: return "SmartTerrainTaskGamePathWalk";
    case eStateSmartTerrainTaskLevelPathWalk: return "SmartTerrainTaskLevelPathWalk";
    case eStateSmartTerrainTaskWaitCapture: return "SmartTerrainTaskWaitCapture";
    case eStateCustom_Vampire: return "Custom_Vampire";
    case eStateVampire_ApproachEnemy: return "Vampire_ApproachEnemy";
    case eStateVampire_Execute: return "Vampire_Execute";
    case eStateVampire_RunAway: return "Vampire_RunAway";
    case eStateVampire_Hide: return "Vampire_Hide";
    case eStateBurerAttack_Tele: return "BurerAttack_Tele";
    case eStateBurerAttack_Gravi: return "BurerAttack_Gravi";
    case eStateBurerAttack_RunAround: return "BurerAttack_RunAround";
    case eStateBurerAttack_FaceEnemy: return "BurerAttack_FaceEnemy";
    case eStateBurerAttack_Melee: return "BurerAttack_Melee";
    case eStateBurerAttack_Shield: return "BurerAttack_Shield";
    case eStateBurerAttack_AntiAim: return "BurerAttack_AntiAim";
    case eStatePredator_MoveToCover: return "Predator_MoveToCover";
    case eStatePredator_LookOpenPlace: return "Predator_LookOpenPlace";
    case eStatePredator_Camp: return "Predator_Camp";
    case eStatePredator: return "Predator";
    case eStateUnknown: return "Unknown";
    default: return "Undefined State";
    }
}
