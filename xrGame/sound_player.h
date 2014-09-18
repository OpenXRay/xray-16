////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_player.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sound player
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "random32.h"
#include "ai_sounds.h"
#include "associative_vector.h"

class CSoundPlayer : public CRandom32 {
public:
	struct CSoundParams {
		u32										m_priority;
		u32										m_synchro_mask;
		shared_str								m_bone_name;
	};

	struct CSoundCollectionParams {
		shared_str								m_sound_prefix;
		shared_str								m_sound_player_prefix;
		u32										m_max_count;
		ESoundTypes								m_type;

		IC		bool	operator==	(const CSoundCollectionParams &object) const
		{
			if (m_sound_prefix != object.m_sound_prefix)
				return	(false);

			if (m_sound_player_prefix != object.m_sound_player_prefix)
				return	(false);

			if (m_max_count != object.m_max_count)
				return	(false);

			if (m_type != object.m_type)
				return	(false);

			return		(true);
		}
	};

	struct CSoundCollectionParamsFull : 
		public CSoundParams,
		public CSoundCollectionParams
	{
		CSound_UserDataPtr	m_data;
	};

	struct CSoundCollection : public CRandom32 {
		xr_vector<ref_sound*>					m_sounds;
		u32										m_last_sound_id;

							CSoundCollection	(const CSoundCollectionParams &params);
							~CSoundCollection	();
		IC	ref_sound		*add				(ESoundTypes type, LPCSTR name) const;
			const ref_sound	&random				(const u32 &id);
	};

	struct CSoundSingle : public CSoundParams {
		ref_sound								*m_sound;
		u32										m_start_time;
		u32										m_stop_time;
		bool									m_started;
		u16										m_bone_id;

				CSoundSingle					()
		{
			m_started							= false;
		}

				void	destroy					()
		{
			VERIFY								(m_sound);
			if (m_sound->_feedback())
				m_sound->stop					();

			xr_delete							(m_sound);
		}

				void	play_at_pos				(CObject *object, const Fvector &position)
		{
			m_sound->play_at_pos				(object,position);
			m_started							= true;
		}

		IC		bool	started					() const
		{
			return								(m_started);
		}
	};

	struct CInappropriateSoundPredicate {
		u32										m_sound_mask;

						CInappropriateSoundPredicate	(u32 sound_mask) : m_sound_mask(sound_mask)
		{
		}

		bool			operator()				(CSoundSingle &sound)
		{
			VERIFY		(sound.m_sound);
			bool		result = 
				(sound.m_synchro_mask & m_sound_mask) || 
				(
					!sound.m_sound->_feedback() && 
					(sound.m_stop_time <= Device.dwTimeGlobal)
				);
			if (result)
				sound.destroy					();
			return								(result);
		}
	};

public:
	typedef std::pair<CSoundCollectionParamsFull,CSoundCollection*>	SOUND_COLLECTION;
	typedef associative_vector<u32,SOUND_COLLECTION>				SOUND_COLLECTIONS;

private:
	SOUND_COLLECTIONS							m_sounds;
	xr_vector<CSoundSingle>						m_playing_sounds;
	u32											m_sound_mask;
	CObject										*m_object;
	shared_str									m_sound_prefix;

	IC		Fvector		compute_sound_point			(const CSoundSingle &sound);
			void		remove_inappropriate_sounds	(u32 sound_mask);
			void		update_playing_sounds		();
			bool		check_sound_legacy			(u32 internal_type) const;

public:
						CSoundPlayer				(CObject *object);
	virtual				~CSoundPlayer				();
	virtual	void		reinit						();
	virtual	void		reload						(LPCSTR section);
			void		unload						();
			u32			add							(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name, CSound_UserDataPtr data = 0);
			void		remove						(u32 internal_type);
			void		clear						();
			void		play						(u32 internal_type, u32 max_start_time = 0, u32 min_start_time = 0, u32 max_stop_time = 0, u32 min_stop_time = 0, u32 id = u32(-1));
			void		update						(float time_delta);
	IC		void		set_sound_mask				(u32 sound_mask);
	IC		void		remove_active_sounds		(u32 sound_mask);
	IC	const xr_vector<CSoundSingle>&playing_sounds() const;
	IC		u32			active_sound_count			(bool only_playing = false) const;
			bool		need_bone_data				() const;
	IC	const SOUND_COLLECTIONS &objects			() const;
	IC		bool		active_sound_type			(u32 synchro_mask) const;
	IC		void		sound_prefix				(const shared_str &sound_prefix);
	IC	const shared_str &sound_prefix				() const;
};

#include "sound_player_inline.h"