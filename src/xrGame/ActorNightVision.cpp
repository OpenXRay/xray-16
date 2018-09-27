#include "stdafx.h"

#include "Actor.h"
#include "level.h"
#include "actorEffector.h"
#include "ai_sounds.h"

#include "ActorNightVision.h"

CNightVisionEffector::CNightVisionEffector(const shared_str& section, u16 effID)
{
	effectorID = effID;
	m_sounds.LoadSound(section.c_str(), "snd_night_vision_on", "NightVisionOnSnd", false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section.c_str(), "snd_night_vision_off", "NightVisionOffSnd", false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section.c_str(), "snd_night_vision_idle", "NightVisionIdleSnd", false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section.c_str(), "snd_night_vision_broken", "NightVisionBrokenSnd", false, SOUND_TYPE_ITEM_USING);
}

void CNightVisionEffector::Start(const shared_str& sect, CActor* pA, bool play_sound)
{
	AddEffector(pA, effectorID, sect);
	if (play_sound)
	{
		PlaySounds(eStartSound);
		PlaySounds(eIdleSound);
	}
}

void CNightVisionEffector::Stop(const float factor, bool play_sound)
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentControlEntity());
	if (!pActor)		return;
	CEffectorPP* pp = pActor->Cameras().GetPPEffector((EEffectorPPType)effectorID);
	if (pp)
	{
		pp->Stop(factor);
		if (play_sound)
			PlaySounds(eStopSound);

		m_sounds.StopSound("NightVisionIdleSnd");
	}
}

bool CNightVisionEffector::IsActive()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentControlEntity());
	if (!pActor)		return false;
	CEffectorPP* pp = pActor->Cameras().GetPPEffector((EEffectorPPType)effectorID);
	return (pp != NULL);
}

void CNightVisionEffector::OnDisabled(CActor* pA, bool play_sound)
{
	if (play_sound)
		PlaySounds(eBrokeSound);
}

void CNightVisionEffector::PlaySounds(EPlaySounds which)
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentControlEntity());
	if (!pActor)
		return;

	bool bPlaySoundFirstPerson = !!pActor->HUDview();
	switch (which)
	{
	case eStartSound:
	{
		m_sounds.PlaySound("NightVisionOnSnd", pActor->Position(), NULL, bPlaySoundFirstPerson);
	}break;
	case eStopSound:
	{
		m_sounds.PlaySound("NightVisionOffSnd", pActor->Position(), NULL, bPlaySoundFirstPerson);
	}break;
	case eIdleSound:
	{
		m_sounds.PlaySound("NightVisionIdleSnd", pActor->Position(), NULL, bPlaySoundFirstPerson, true);
	}break;
	case eBrokeSound:
	{
		m_sounds.PlaySound("NightVisionBrokenSnd", pActor->Position(), NULL, bPlaySoundFirstPerson);
	}break;
	default: NODEFAULT;
	}
}
