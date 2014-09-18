////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat+space.h
//	Created 	: 27.07.2004
//  Modified 	: 27.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Space for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace RatSpace {
	enum ERatSounds {
		eRatSoundDie = u32(0),
		eRatSoundInjuring,
		eRatSoundAttack,
		eRatSoundVoice,
		eRatSoundEat,
		eRatSoundDummy = u32(-1),
	};

	enum ERatSoundMasks {
		eRatSoundMaskAnySound	= u32(0),
		eRatSoundMaskDie		= u32(-1),
		eRatSoundMaskInjuring	= u32(-1),
		eRatSoundMaskVoice		= u32(1 << 31) | 1,
		eRatSoundMaskAttack		= u32(1 << 30) | 2,
		eRatSoundMaskEat		= u32(1 << 30) | 4,
		eRatSoundMaskDummy		= u32(-1),
	};
}