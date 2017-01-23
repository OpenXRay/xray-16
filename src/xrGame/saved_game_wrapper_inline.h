////////////////////////////////////////////////////////////////////////////
//	Module 		: saved_game_wrapper_inline.h
//	Created 	: 21.02.2006
//  Modified 	: 21.02.2006
//	Author		: Dmitriy Iassenev
//	Description : saved game wrapper class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

inline const CSavedGameWrapper::_TIME_ID &CSavedGameWrapper::game_time	() const
{
	return	(m_game_time);
}

inline const CSavedGameWrapper::_LEVEL_ID &CSavedGameWrapper::level_id	() const
{
	return	(m_level_id);
}

inline LPCSTR CSavedGameWrapper::level_name								() const
{
	return	(m_level_name.c_str());
}

inline const float &CSavedGameWrapper::actor_health						() const
{
	return	(m_actor_health);
}
