#pragma once
#include "xrCommon/xr_string.h"

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
xr_string make_xrstr(EMonsterState state) noexcept;
