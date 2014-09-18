////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_player_inline.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sound player inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void CSoundPlayer::set_sound_mask		(u32 sound_mask)
{
	m_sound_mask						= sound_mask;
	remove_inappropriate_sounds			(m_sound_mask);
}

IC	void CSoundPlayer::remove_active_sounds	(u32 sound_mask)
{
	u32									save_sound_mask = m_sound_mask;
	set_sound_mask						(sound_mask);
	set_sound_mask						(save_sound_mask);
}

IC	const xr_vector<CSoundPlayer::CSoundSingle> &CSoundPlayer::playing_sounds() const
{
	return								(m_playing_sounds);
}

IC	u32	CSoundPlayer::active_sound_count(bool only_playing) const
{
	u32									count = 0;
	xr_vector<CSoundSingle>::const_iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::const_iterator	E = m_playing_sounds.end();
	if (!only_playing) {
		for ( ; I != E; ++I)
			if ((*I).m_sound->_feedback() || ((*I).m_start_time <= Device.dwTimeGlobal))
				++count;
	}
	else {
		for ( ; I != E; ++I)
			if ((*I).m_sound->_feedback())
				++count;
	}
	return								(count);
}

IC	ref_sound *CSoundPlayer::CSoundCollection::add	(ESoundTypes type, LPCSTR name) const
{
	ref_sound				*temp = xr_new<ref_sound>();
	temp->create			(name,st_Effect,type);
	if (!temp->_p)
		return				(0);
	return					(temp);
}

IC	const CSoundPlayer::SOUND_COLLECTIONS &CSoundPlayer::objects	() const
{
	return				(m_sounds);
}

IC	bool CSoundPlayer::active_sound_type							(u32 synchro_mask) const
{
	xr_vector<CSoundSingle>::const_iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::const_iterator	E = m_playing_sounds.end();
	for ( ; I != E; ++I) {
		if ((*I).m_sound->_feedback() || ((*I).m_start_time <= Device.dwTimeGlobal)) {
			if (synchro_mask == (*I).m_synchro_mask) {
				return	(true);
			}
		}
	}

	return				(false);
}

IC	void CSoundPlayer::sound_prefix									(const shared_str &sound_prefix)
{
	m_sound_prefix		= sound_prefix;
	if (!m_sound_prefix)
		m_sound_prefix	= "";
}

IC	const shared_str &CSoundPlayer::sound_prefix					() const
{
	return				(m_sound_prefix);
}
