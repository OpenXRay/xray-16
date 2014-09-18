#pragma once


// monster base sound types
namespace MonsterSound {
	enum EType {
		eMonsterSoundBase			= u32(0),
		eMonsterSoundIdle			= eMonsterSoundBase | 1,
		eMonsterSoundEat			= eMonsterSoundBase | 2,
		eMonsterSoundAggressive		= eMonsterSoundBase | 3,
		eMonsterSoundAttackHit		= eMonsterSoundBase | 4,
		eMonsterSoundTakeDamage		= eMonsterSoundBase | 5,
		eMonsterSoundStrike			= eMonsterSoundBase | 6,

		eMonsterSoundDie			= eMonsterSoundBase | 7,
		eMonsterSoundDieInAnomaly	= eMonsterSoundBase | 8,

		eMonsterSoundThreaten		= eMonsterSoundBase | 9,
		eMonsterSoundSteal			= eMonsterSoundBase | 10,
		eMonsterSoundPanic			= eMonsterSoundBase | 11,
		eMonsterSoundIdleDistant	= eMonsterSoundBase | 12,

		eMonsterSoundScript			= u32(1) << 7,
		eMonsterSoundCustom			= eMonsterSoundScript << 7,
		eMonsterSoundDummy			= u32(-1),
	};

	enum EPriority {
		eCriticalPriority	= u32(1),
		eHighPriority		= u32(1) << 3,
		eNormalPriority		= u32(1) << 7,
		eLowPriority		= u32(1) << 15
	};

	enum EVirtualChannels {
		eBaseChannel		= u32(1) << 7,
		eChannelIndependent	= u32(1) << 15,	// can be played in any time with any conditions (need shift operator for every id)
		eCaptureAllChannels = u32(-1),		// play only this one
	};
};

#define DEFAULT_SAMPLE_COUNT	16
