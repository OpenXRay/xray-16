////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_space.h
//	Created 	: 10.06.2004
//  Modified 	: 10.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement manager space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace StalkerMovement {
	enum eVelocities {
		eVelocityStanding								= u32(1) <<  0,
		eVelocityWalk									= u32(1) <<  1,
		eVelocityRun									= u32(1) <<  2,
		eVelocityMovementType							= eVelocityStanding | eVelocityWalk | eVelocityRun,

		eVelocityStand									= u32(1) <<  3,
		eVelocityCrouch									= u32(1) <<  4,
		eVelocityBodyState								= eVelocityStand | eVelocityCrouch,

		eVelocityDanger									= u32(1) <<  6,
		eVelocityFree									= u32(1) <<  7,
		eVelocityPanic									= u32(1) <<  8,
		eVelocityMentalState							= eVelocityDanger | eVelocityFree | eVelocityPanic,

		eVelocityPositiveVelocity						= u32(1) << 31,
		eVelocityNegativeVelocity						= u32(1) << 30,

		eVelocityStandingFreeStand						= eVelocityStanding | eVelocityFree | eVelocityStand,
		eVelocityStandingPanicStand						= eVelocityStanding | eVelocityPanic | eVelocityStand,
		eVelocityStandingDangerStand					= eVelocityStanding | eVelocityDanger | eVelocityStand,

		eVelocityStandingFreeCrouch						= eVelocityStanding | eVelocityFree | eVelocityCrouch,
		eVelocityStandingPanicCrouch					= eVelocityStanding | eVelocityPanic | eVelocityCrouch,
		eVelocityStandingDangerCrouch					= eVelocityStanding | eVelocityDanger | eVelocityCrouch,

		eVelocityWalkFree								= eVelocityWalk | eVelocityFree | eVelocityStand,
		eVelocityWalkDangerStand						= eVelocityWalk | eVelocityDanger | eVelocityStand,
		eVelocityWalkDangerCrouch						= eVelocityWalk | eVelocityDanger | eVelocityCrouch,
		eVelocityRunFree								= eVelocityRun | eVelocityFree | eVelocityStand,
		eVelocityRunDangerStand							= eVelocityRun | eVelocityDanger | eVelocityStand,
		eVelocityRunDangerCrouch						= eVelocityRun | eVelocityDanger | eVelocityCrouch,
		eVelocityRunPanicStand							= eVelocityRun | eVelocityPanic | eVelocityStand,

		eVelocityStandingFreeCrouchPositive				= eVelocityStandingFreeCrouch			| eVelocityPositiveVelocity,
		eVelocityStandingPanicCrouchPositive			= eVelocityStandingPanicCrouch			| eVelocityPositiveVelocity,
		eVelocityStandingDangerCrouchPositive			= eVelocityStandingDangerCrouch			| eVelocityPositiveVelocity,

		eVelocityWalkFreePositive						= eVelocityWalkFree						| eVelocityPositiveVelocity,
		eVelocityWalkDangerStandPositive				= eVelocityWalkDangerStand				| eVelocityPositiveVelocity,
		eVelocityWalkDangerCrouchPositive				= eVelocityWalkDangerCrouch				| eVelocityPositiveVelocity,
		eVelocityRunFreePositive						= eVelocityRunFree						| eVelocityPositiveVelocity,
		eVelocityRunDangerStandPositive					= eVelocityRunDangerStand				| eVelocityPositiveVelocity,
		eVelocityRunDangerCrouchPositive				= eVelocityRunDangerCrouch				| eVelocityPositiveVelocity,
		eVelocityRunPanicStandPositive					= eVelocityRunPanicStand				| eVelocityPositiveVelocity,

		eVelocityStandingFreeCrouchNegative				= eVelocityStandingFreeCrouch			| eVelocityNegativeVelocity,
		eVelocityStandingPanicCrouchNegative			= eVelocityStandingPanicCrouch			| eVelocityNegativeVelocity,
		eVelocityStandingDangerCrouchNegative			= eVelocityStandingDangerCrouch			| eVelocityNegativeVelocity,

		eVelocityWalkFreeNegative						= eVelocityWalkFree						| eVelocityNegativeVelocity,
		eVelocityWalkDangerStandNegative				= eVelocityWalkDangerStand				| eVelocityNegativeVelocity,
		eVelocityWalkDangerCrouchNegative				= eVelocityWalkDangerCrouch				| eVelocityNegativeVelocity,
		eVelocityRunFreeNegative						= eVelocityRunFree						| eVelocityNegativeVelocity,
		eVelocityRunDangerStandNegative					= eVelocityRunDangerStand				| eVelocityNegativeVelocity,
		eVelocityRunDangerCrouchNegative				= eVelocityRunDangerCrouch				| eVelocityNegativeVelocity,
		eVelocityRunPanicStandNegative					= eVelocityRunPanicStand				| eVelocityNegativeVelocity,
	};
};