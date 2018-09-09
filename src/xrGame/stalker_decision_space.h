////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_decision_space.h
//	Created 	: 30.03.2004
//  Modified 	: 30.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker decision space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace StalkerDecisionSpace
{
enum EWorldProperties : u32
{
    eWorldPropertyAlive = u32(0),
    eWorldPropertyDead,
    eWorldPropertyAlreadyDead,

    eWorldPropertyALife,
    eWorldPropertyPuzzleSolved,

    eWorldPropertySmartTerrainTask,
    eWorldPropertyItems,
    eWorldPropertyEnemy,
    eWorldPropertyDanger,

    eWorldPropertyItemToKill,
    eWorldPropertyFoundItemToKill,
    eWorldPropertyItemCanKill,
    eWorldPropertyFoundAmmo,
    eWorldPropertyReadyToKill,
    eWorldPropertyReadyToDetour,
    eWorldPropertySeeEnemy,
    eWorldPropertyEnemySeeMe,
    eWorldPropertyPanic,
    eWorldPropertyInCover,
    eWorldPropertyLookedOut,
    eWorldPropertyPositionHolded,
    eWorldPropertyEnemyDetoured,
    eWorldPropertyUseSuddenness,
    eWorldPropertyPureEnemy,
    eWorldPropertyUseCrouchToLookOut,
    eWorldPropertyEnemyWounded,
    eWorldPropertyWoundedEnemyReached,
    eWorldPropertyWoundedEnemyPrepared,
    eWorldPropertyPlayerOnThePath,
    eWorldPropertyCriticallyWounded,
    eWorldPropertyEnemyCriticallyWounded,
    eWorldPropertyWoundedEnemyAimed,
    eWorldPropertyPausedAfterKill,
    eWorldPropertyKilledWounded,
    eWorldPropertyTooFarToKillEnemy,
    eWorldPropertyEnemyLocationReached,
    eWorldPropertyAmbushLocationReached,
    eWorldPropertyStartedToThrowGrenade,

    eWorldPropertyDangerUnknown,
    eWorldPropertyDangerInDirection,
    eWorldPropertyDangerGrenade,
    eWorldPropertyDangerBySound,

    eWorldPropertyCoverActual,
    eWorldPropertyCoverReached,
    eWorldPropertyLookedAround,
    eWorldPropertyGrenadeExploded,

    eWorldPropertyAnomaly,
    eWorldPropertyInsideAnomaly,

    eWorldPropertyShouldThrowGrenade,

    eWorldPropertyLowCover,

    eWorldPropertyInSmartCover,

    eWorldPropertyLoopholeIdle,
    eWorldPropertyLoopholeActual,
    eWorldPropertyLoopholeFire,
    eWorldPropertyLoopholeFireNoLookout,
    eWorldPropertyExitSmartCover,
    eWorldPropertySmartCoverEntered,
    eWorldPropertySmartCoverActual,
    eWorldPropertyReadyToLookout,
    eWorldPropertyReadyToIdle,
    eWorldPropertyReadyToFire,
    eWorldPropertyReadyToFireNoLookout,
    eWorldPropertyLoopholeLastHitWasLongAgo,
    eWorldPropertyLoopholeCanLookout,
    eWorldPropertyLoopholeCanFire,
    eWorldPropertyLoopholeCanFireNoLookout,
    eWorldPropertyLoopholeCanStayIdle,
    eWorldPropertyLoopholeExitable,
    eWorldPropertyPlannerHasTarget,
    eWorldPropertyLoopholeCanExitWithAnimation,
    eWorldPropertyLoopholeUseDefaultBehaviour,
    eWorldPropertyLoopholeCanFireAtEnemy,
    eWorldPropertyLoopholeTooMuchTimeFiring,
    eWorldPropertyStayIdle,

    eWorldPropertyScript,
    eWorldPropertyDummy = u32(-1),
};

enum EWorldOperators
{
    // death
    eWorldOperatorDead = u32(0),
    eWorldOperatorDying,

    // alife
    eWorldOperatorGatherItems,
    eWorldOperatorALifeEmulation,
    eWorldOperatorSmartTerrainTask,

    // alife : tasks
    eWorldOperatorSolveZonePuzzle,
    eWorldOperatorReachTaskLocation,
    eWorldOperatorAccomplishTask,
    eWorldOperatorReachCustomerLocation,
    eWorldOperatorCommunicateWithCustomer,

    // anomaly
    eWorldOperatorGetOutOfAnomaly,
    eWorldOperatorDetectAnomaly,

    // combat
    eWorldOperatorGetItemToKill,
    eWorldOperatorFindItemToKill,
    eWorldOperatorMakeItemKilling,
    eWorldOperatorFindAmmo,

    eWorldOperatorAimEnemy,
    eWorldOperatorGetReadyToKill,
    eWorldOperatorGetReadyToDetour,
    eWorldOperatorKillEnemy,
    eWorldOperatorRetreatFromEnemy,
    eWorldOperatorTakeCover,
    eWorldOperatorLookOut,
    eWorldOperatorHoldPosition,
    eWorldOperatorGetDistance,
    eWorldOperatorDetourEnemy,
    eWorldOperatorSearchEnemy,
    eWorldOperatorHideFromGrenade,
    eWorldOperatorSuddenAttack,
    eWorldOperatorKillEnemyIfNotVisible,
    eWorldOperatorReachWoundedEnemy,
    eWorldOperatorAimWoundedEnemy,
    eWorldOperatorPrepareWoundedEnemy,
    eWorldOperatorKillWoundedEnemy,
    eWorldOperatorPostCombatWait,
    eWorldOperatorKillEnemyIfPlayerOnThePath,
    eWorldOperatorCriticallyWounded,
    eWorldOperatorKillEnemyIfCriticallyWounded,
    eWorldOperatorPauseAfterKill,
    eWorldOperatorThrowGrenade,
    eWorldOperatorRunToCover,
    eWorldOperatorWaitInCover,
    eWorldOperatorReachEnemyLocation,
    eWorldOperatorReachAmbushLocation,
    eWorldOperatorHoldAmbushLocation,
    eWorldOperatorLowCover,

    eWorldOperatorInSmartCover,
    eWorldOperatorSmartCoverEnter,
    eWorldOperatorChangeLoophole,
    eWorldOperatorGoToLoophole,
    eWorldOperatorExitSmartCover,
    eWorldOperatorAnimation,
    eWorldOperatorSmartCoverIdle,
    eWorldOperatorSmartCoverLookout,
    eWorldOperatorSmartCoverFire,
    eWorldOperatorSmartCoverReload,
    eWorldOperatorSmartCoverFireNoLookout,
    eWorldOperatorSmartCoverExit,
    eWorldOperatorSmartCoverIdle2Lookout,
    eWorldOperatorSmartCoverLookout2Idle,
    eWorldOperatorSmartCoverIdle2Fire,
    eWorldOperatorSmartCoverFire2Idle,
    eWorldOperatorSmartCoverIdle2FireNoLookout,
    eWorldOperatorSmartCoverFireNoLookout2Idle,
    eWorldOperatorLoopholeTargetIdle,
    eWorldOperatorLoopholeTargetLookout,
    eWorldOperatorLoopholeTargetFire,
    eWorldOperatorLoopholeTargetFireNoLookout,
    eWorldOperatorLoopholeTargetDefaultBehaviour,
    eWorldOperatorWaitAfterSmartCoverExit,

    // danger
    eWorldOperatorDangerUnknownPlanner,
    eWorldOperatorDangerInDirectionPlanner,
    eWorldOperatorDangerGrenadePlanner,
    eWorldOperatorDangerBySoundPlanner,

    eWorldOperatorDangerUnknownTakeCover,
    eWorldOperatorDangerUnknownLookAround,
    eWorldOperatorDangerUnknownSearchEnemy,

    eWorldOperatorDangerInDirectionTakeCover,
    eWorldOperatorDangerInDirectionLookOut,
    eWorldOperatorDangerInDirectionHoldPosition,
    eWorldOperatorDangerInDirectionDetourEnemy,
    eWorldOperatorDangerInDirectionSearchEnemy,

    eWorldOperatorDangerGrenadeTakeCover,
    eWorldOperatorDangerGrenadeWaitForExplosion,
    eWorldOperatorDangerGrenadeTakeCoverAfterExplosion,
    eWorldOperatorDangerGrenadeLookAround,
    eWorldOperatorDangerGrenadeSearch,

    // global
    eWorldOperatorDeathPlanner,
    eWorldOperatorALifePlanner,
    eWorldOperatorCombatPlanner,
    eWorldOperatorAnomalyPlanner,
    eWorldOperatorDangerPlanner,

    // script
    eWorldOperatorScript,
    eWorldOperatorDummy = u32(-1),
};

enum ESightActionType
{
    eSightActionTypeWatchItem = u32(0),
    eSightActionTypeWatchEnemy,
    eSightActionTypeDummy = u32(-1),
};
};
