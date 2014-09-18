#pragma once

namespace MonsterMovement {

	enum EMovementParameters {
		eVelocityParameterIdle			= u32(1) <<	 1,
		eVelocityParameterStand			= u32(1) <<  4,
		eVelocityParameterWalkNormal	= u32(1) <<  3,
		eVelocityParameterRunNormal		= u32(1) <<  2,

		eVelocityParameterWalkDamaged	= u32(1) <<  5,
		eVelocityParameterRunDamaged	= u32(1) <<  6,
		eVelocityParameterSteal			= u32(1) <<  7,
		eVelocityParameterDrag			= u32(1) <<  8,
		eVelocityParameterInvisible		= u32(1) <<	 9,
		eVelocityParameterRunAttack		= u32(1) <<	 10,
		eVelocityParameterWalkSmelling	= u32(1) <<	 11,
		eVelocityParameterWalkGrowl		= u32(1) <<	 12,

		eVelocityParamsWalkGrowl		= eVelocityParameterStand		| eVelocityParameterWalkGrowl,
		eVelocityParamsWalkSmelling		= eVelocityParameterStand		| eVelocityParameterWalkSmelling,
		eVelocityParamsWalk				= eVelocityParameterStand		| eVelocityParameterWalkNormal,
		eVelocityParamsWalkDamaged		= eVelocityParameterStand		| eVelocityParameterWalkDamaged,
		eVelocityParamsRun				= eVelocityParameterStand		| eVelocityParameterWalkNormal	| eVelocityParameterRunNormal,
		eVelocityParamsRunDamaged		= eVelocityParameterStand		| eVelocityParameterWalkDamaged | eVelocityParameterRunDamaged,
		eVelocityParamsAttackNorm		= eVelocityParameterStand		| eVelocityParameterWalkNormal	| eVelocityParameterRunNormal,
		eVelocityParamsAttackDamaged	= eVelocityParameterStand		| eVelocityParameterWalkDamaged | eVelocityParameterRunDamaged,
		eVelocityParamsSteal			= eVelocityParameterStand		| eVelocityParameterSteal,
		eVelocityParamsDrag				= eVelocityParameterStand		| eVelocityParameterDrag,
		eVelocityParamsInvisible		= eVelocityParameterInvisible	| eVelocityParameterStand,
		eVelocityParamsRunAttack		= eVelocityParameterRunAttack	| eVelocityParameterStand, 


		eVelocityParameterCustom		= u32(1) <<	 12,
	};

	enum EMovementParametersChimera {
		eChimeraVelocityParameterPrepare			= eVelocityParameterCustom << 1,
		eChimeraVelocityParameterJumpGround			= eVelocityParameterCustom << 2,
	};

	enum EMovementParametersSnork {
		eSnorkVelocityParameterJumpGround		= eVelocityParameterCustom << 2,
	};

	enum EMovementParametersBloodsucker {
		eBloodsuckerVelocityParameterJumpGround		= eVelocityParameterCustom << 2,
	};

	enum EMovementParametersController {
		eControllerVelocityParameterMoveFwd		= eVelocityParameterCustom << 1,
		eControllerVelocityParameterMoveBkwd	= eVelocityParameterCustom << 2,

		eControllerVelocityParamsMoveFwd		= eControllerVelocityParameterMoveFwd	| eVelocityParameterStand,
		eControllerVelocityParamsMoveBkwd		= eControllerVelocityParameterMoveBkwd	| eVelocityParameterStand,
	};

	enum EMovementParametersGiant {
		eGiantVelocityParameterJumpPrepare		= eVelocityParameterCustom << 1,
		eGiantVelocityParameterJumpGround		= eVelocityParameterCustom << 2,
	};

};


