////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script sound class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_sound.h"
#include "script_game_object.h"
#include "gameobject.h"
#include "ai_space.h"
#include "script_engine.h"

CScriptSound::CScriptSound				(LPCSTR caSoundName, ESoundTypes sound_type)
{
	m_caSoundToPlay			= caSoundName;
	string_path				l_caFileName;
	VERIFY(::Sound)	;
	if (FS.exist(l_caFileName,"$game_sounds$",caSoundName,".ogg"))
		m_sound.create		(caSoundName,st_Effect,sound_type);
	else
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"File not found \"%s\"!",l_caFileName);
}

CScriptSound::~CScriptSound		()
{
	THROW3					(!m_sound._feedback(),"playing sound is not completed, but is destroying",m_sound._handle() ? m_sound._handle()->file_name() : "unknown");
	m_sound.destroy			();
}

Fvector CScriptSound::GetPosition() const
{
	VERIFY				(m_sound._handle());
	const CSound_params	*l_tpSoundParams = m_sound.get_params();
	if (l_tpSoundParams)
		return			(l_tpSoundParams->position);
	else {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"Sound was not launched, can't get position!");
		return			(Fvector().set(0,0,0));
	}
}

void CScriptSound::Play			(CScriptGameObject *object, float delay, int flags)
{
	THROW3						(m_sound._handle(), "There is no sound", *m_caSoundToPlay);
//	Msg							("%6d : CScriptSound::Play (%s), delay %f, flags %d",Device.dwTimeGlobal,m_sound._handle()->file_name(),delay,flags);
	m_sound.play				((object) ? &object->object() : NULL, flags, delay);
}

void CScriptSound::PlayAtPos		(CScriptGameObject *object, const Fvector &position, float delay, int flags)
{
	THROW3						(m_sound._handle(),"There is no sound",*m_caSoundToPlay);
//	Msg							("%6d : CScriptSound::Play (%s), delay %f, flags %d",m_sound._handle()->file_name(),delay,flags);
	m_sound.play_at_pos			((object) ? &object->object() : NULL, position,flags,delay);
}

void CScriptSound::PlayNoFeedback	(CScriptGameObject *object,	u32 flags/*!< Looping */, float delay/*!< Delay */, Fvector pos, float vol)
{
	THROW3						(m_sound._handle(),"There is no sound",*m_caSoundToPlay);
	m_sound.play_no_feedback	((object) ? &object->object() : NULL, flags,delay,&pos,&vol);
}
