#pragma once

namespace ControlCom {
	enum EControlType {
		// 1st level
		eControlMovement	= u32(0),	// linear velocity
		eControlPath,					// path builder
		eControlDir,					// model direction
		eControlAnimation,				// animation manager
//		eControlSound,					// sound manager

		// 2nd level
		eControlSequencer,				// capture: anim
		eControlTripleAnimation,		// capture: anim

		//// 3rd level 
		eControlJump,					// capture: path, movement, triple_anim     disable : fsm, dir
		eControlRotationJump,			// capture: path, movement, sequencer, dir
		eControlRunAttack,				// capture: path, movement, sequencer
		eControlThreaten,				
		eControlMeleeJump,				// capture: path, movement, sequencer, dir

		eControlAnimationBase,
		eControlMovementBase,
		eControlPathBase,
		eControlDirBase,

		eControlCustom,

		eComCustom1,
		eComCriticalWound,
		eAntiAim,
		
		eControllersCount,

		eControlInvalid		= u32(-1)
	};

	struct	IComData	{};
	struct	IEventData	{};

	enum EEventType {
		eventAnimationStart = u32(0),
		eventAnimationEnd,
		eventLegsAnimationEnd,
		eventTorsoAnimationEnd,
		eventAnimationSignal,
		eventSoundStart,
		eventSoundEnd,
		eventParticlesStart,
		eventParticlesEnd,
		eventStep,
		eventTAChange,
		eventVelocityBounce,
		eventSequenceEnd,
		eventRotationEnd,
		eventTravelPointChange,
		eventPathBuilt,
		eventJumpEnd,
		eventRotationJumpEnd,
		eventMeleeJumpEnd,
		eventRunAttackEnd,
		eventPathUpdated,
		eventPathSelectorFailed,
		eventThreatenEnd,
		eventCriticalWoundEnd
	};

	enum ECaptureType {
		eCapturePath		= u32(1) << 0,
		eCaptureMovement	= u32(1) << 1,
		eCaptureDir			= u32(1) << 2,
	};
};

