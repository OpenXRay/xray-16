////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_explosive_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger explosive class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CDangerExplosive::CDangerExplosive	(const CExplosive *grenade, const CGameObject *game_object, CAI_Stalker *reactor, u32 time)
{
	m_grenade	= grenade;
	m_game_object = game_object;
	VERIFY		(!m_grenade || m_game_object);
	m_reactor	= reactor;
	m_time		= time;
}

IC	bool CDangerExplosive::operator==	(const CExplosive *grenade) const
{
	return		(m_grenade == grenade);
}
