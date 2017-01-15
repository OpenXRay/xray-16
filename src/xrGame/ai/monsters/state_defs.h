#pragma once

enum EMonsterState
{
    eGlobalState = u32(1) << 15,

    // -------------------------------------------------------------

    eStateRest = eGlobalState << 1,

    eStateRest_WalkGraphPoint = eStateRest | 1,
    eStateRest_Idle = eStateRest | 2,
    eStateRest_Fun = eStateRest | 3,
    eStateRest_Sleep = eStateRest | 4,
    eStateRest_MoveToHomePoint = eStateRest | 5,
    eStateRest_WalkToCover = eStateRest | 6,
    eStateRest_LookOpenPlace = eStateRest | 7,

    // -------------------------------------------------------------

    eStateEat = eGlobalState << 2,

    eStateEat_CorpseApproachRun = eStateEat | 1,
    eStateEat_CorpseApproachWalk = eStateEat | 2,
    eStateEat_CheckCorpse = eStateEat | 3,
    eStateEat_Eat = eStateEat | 4,
    eStateEat_WalkAway = eStateEat | 5,
    eStateEat_Rest = eStateEat | 6,
    eStateEat_Drag = eStateEat | 7,

    // -------------------------------------------------------------

    eStateAttack = eGlobalState << 3,

    eStateAttack_Run = eStateAttack | 1,
    eStateAttack_Melee = eStateAttack | 2,
    eStateAttack_RunAttack = eStateAttack | 3,
    eStateAttack_RunAway = eStateAttack | 4,
    eStateAttack_FindEnemy = eStateAttack | 5,
    eStateAttack_Steal = eStateAttack | 6,

    eStateAttack_AttackHidden = eStateAttack | 7,

    eStateAttack_Hide = eStateAttack | 9,

    eStateAttack_HideInCover = eStateAttack | 10,
    eStateAttack_MoveOut = eStateAttack | 11,
    eStateAttack_CampInCover = eStateAttack | 12,
    eStateAttack_ControlFire = eStateAttack | 13,
    eStateAttack_ControlTube = eStateAttack | 14,
    eStateAttack_HideInCoverLite = eStateAttack | 15,

    eStateAttackCamp = eStateAttack | 16,
    eStateAttackCamp_Hide = eStateAttack | 17,
    eStateAttackCamp_Camp = eStateAttack | 18,
    eStateAttackCamp_StealOut = eStateAttack | 19,

    eStateAttack_Psy = eStateAttack | 20,
    eStateAttack_MoveToHomePoint = eStateAttack | 21,
    eStateAttack_HomePoint_Hide = eStateAttack | 22,
    eStateAttack_HomePoint_Camp = eStateAttack | 23,
    eStateAttack_HomePoint_LookOpenPlace = eStateAttack | 24,
    eStateAttack_Attack_On_Run = eStateAttack | 25,

    // -------------------------------------------------------------

    eStatePanic = eGlobalState << 4,

    eStatePanic_Run = eStatePanic | 1,
    eStatePanic_FaceUnprotectedArea = eStatePanic | 2,
    eStatePanic_MoveToHomePoint = eStatePanic | 3,

    eStatePanic_HomePoint_Hide = eStatePanic | 4,
    eStatePanic_HomePoint_LookOpenPlace = eStatePanic | 5,
    eStatePanic_HomePoint_Camp = eStatePanic | 6,

    // -------------------------------------------------------------

    eStateHitted = eGlobalState << 5,

    eStateHitted_Hide = eStateHitted | 1,
    eStateHitted_MoveOut = eStateHitted | 2,
    eStateHitted_Home = eStateHitted | 3,

    // -------------------------------------------------------------

    eStateHearDangerousSound = eGlobalState << 6,

    eStateHearDangerousSound_Hide = eStateHearDangerousSound | 1,
    eStateHearDangerousSound_FaceOpenPlace = eStateHearDangerousSound | 2,
    eStateHearDangerousSound_StandScared = eStateHearDangerousSound | 3,
    eStateHearDangerousSound_Home = eStateHearDangerousSound | 4,

    // -------------------------------------------------------------

    eStateHearInterestingSound = eGlobalState << 7,

    eStateHearInterestingSound_MoveToDest = eStateHearInterestingSound | 1,
    eStateHearInterestingSound_LookAround = eStateHearInterestingSound | 2,

    eStateHearHelpSound = eStateHearInterestingSound | 3,
    eStateHearHelpSound_MoveToDest = eStateHearInterestingSound | 4,
    eStateHearHelpSound_LookAround = eStateHearInterestingSound | 5,

    // -------------------------------------------------------------

    eStateControlled = eGlobalState << 8,

    eStateControlled_Follow = eStateControlled | 1,
    eStateControlled_Attack = eStateControlled | 2,

    eStateControlled_Follow_Wait = eStateControlled | 3,
    eStateControlled_Follow_WalkToObject = eStateControlled | 4,

    // -------------------------------------------------------------

    eStateThreaten = eGlobalState << 9,

    // -------------------------------------------------------------

    eStateFindEnemy = eGlobalState << 10,

    eStateFindEnemy_Run = eStateFindEnemy | 1,
    eStateFindEnemy_LookAround = eStateFindEnemy | 2,
    eStateFindEnemy_Angry = eStateFindEnemy | 3,
    eStateFindEnemy_WalkAround = eStateFindEnemy | 4,

    eStateFindEnemy_LookAround_MoveToPoint = eStateFindEnemy | 5,
    eStateFindEnemy_LookAround_LookAround = eStateFindEnemy | 6,
    eStateFindEnemy_LookAround_TurnToPoint = eStateFindEnemy | 7,

    // -------------------------------------------------------------

    eStateSquad = eGlobalState << 11,

    eStateSquad_Rest = eStateSquad | 1,
    eStateSquad_RestFollow = eStateSquad | 2,

    eStateSquad_Rest_Idle = eStateSquad | 3,
    eStateSquad_Rest_WalkAroundLeader = eStateSquad | 4,

    eStateSquad_RestFollow_Idle = eStateSquad | 5,
    eStateSquad_RestFollow_WalkToPoint = eStateSquad | 6,

    // -------------------------------------------------------------

    eStateCustom = eGlobalState << 15,

    eStateBurerScanning = eStateCustom | 1,
    eStateCustomMoveToRestrictor = eStateCustom | 2,

    eStateSmartTerrainTask = eStateCustom | 3,
    eStateSmartTerrainTaskGamePathWalk = eStateCustom | 4,
    eStateSmartTerrainTaskLevelPathWalk = eStateCustom | 5,
    eStateSmartTerrainTaskWaitCapture = eStateCustom | 6,

    // -------------------------------------------------------------
    // custom attack states

    eStateCustom_Vampire = eStateCustom | eStateAttack | 1,

    eStateVampire_ApproachEnemy = eStateCustom | eStateAttack | 2,
    eStateVampire_Execute = eStateCustom | eStateAttack | 3,
    eStateVampire_RunAway = eStateCustom | eStateAttack | 4,
    eStateVampire_Hide = eStateCustom | eStateAttack | 5,

    eStateBurerAttack_Tele = eStateCustom | eStateAttack | 6,
    eStateBurerAttack_Gravi = eStateCustom | eStateAttack | 7,
    eStateBurerAttack_RunAround = eStateCustom | eStateAttack | 8,
    eStateBurerAttack_FaceEnemy = eStateCustom | eStateAttack | 9,
    eStateBurerAttack_Melee = eStateCustom | eStateAttack | 10,
    eStateBurerAttack_Shield = eStateCustom | eStateAttack | 11,
    eStateBurerAttack_AntiAim = eStateCustom | eStateAttack | 12,

    eStatePredator_MoveToCover = eStateCustom | eStateAttack | 13,
    eStatePredator_LookOpenPlace = eStateCustom | eStateAttack | 14,
    eStatePredator_Camp = eStateCustom | eStateAttack | 15,

    eStatePredator = eStateCustom | eStateAttack | 16,

    // -------------------------------------------------------------

    eStateUnknown = u32(-1),
};

#define is_state(state, type) (((state & type) == type) && (state != eStateUnknown))

// Lain: added
inline xr_string make_xrstr(EMonsterState state)
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
