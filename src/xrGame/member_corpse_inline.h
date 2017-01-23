////////////////////////////////////////////////////////////////////////////
//	Module 		: member_corpse_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Member corpse inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CMemberCorpse::CMemberCorpse		(CAI_Stalker *corpse, CAI_Stalker *reactor, u32 time)
{
	m_corpse	= corpse;
	m_reactor	= reactor;
	m_time		= time;
}

IC	bool CMemberCorpse::operator==		(CAI_Stalker *corpse) const
{
	return		(m_corpse == corpse);
}

IC	void CMemberCorpse::reactor			(CAI_Stalker *reactor)
{
	m_reactor	= reactor;
}

IC	CAI_Stalker	*CMemberCorpse::corpse	() const
{
	return		(m_corpse);
}

IC	CAI_Stalker	*CMemberCorpse::reactor	() const
{
	return		(m_reactor);
}

IC	u32	CMemberCorpse::time				() const
{
	return		(m_time);
}
