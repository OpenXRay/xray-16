#pragma once
#include "hudsound.h"

class CNightVisionEffector
{
	HUD_SOUND_COLLECTION	m_sounds;
public:
	enum EPlaySounds{
		eStartSound = 0,
		eStopSound,
		eIdleSound,
		eBrokeSound
	};
	u16			effectorID;
	CNightVisionEffector(const shared_str& sect, u16 effID = effNightvision);
	void		Start(const shared_str& sect, CActor* pA, bool play_sound = true);
	void		Stop(const float factor, bool play_sound = true);
	bool		IsActive();
	void		OnDisabled(CActor* pA, bool play_sound = true);
	void		PlaySounds(EPlaySounds which);
};