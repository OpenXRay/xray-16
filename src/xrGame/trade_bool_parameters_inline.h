////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_bool_parameters.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade bool parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeBoolParameters::CTradeBoolParameters	()
{
}

IC	void CTradeBoolParameters::clear			()
{
	m_sections.clear			();
}

IC	void CTradeBoolParameters::disable			(const shared_str &section)
{
	SECTIONS::iterator			I = std::find(m_sections.begin(),m_sections.end(),section);
	VERIFY						(I == m_sections.end());
	m_sections.push_back		(section);
}

IC	bool CTradeBoolParameters::disabled			(const shared_str &section) const
{
	SECTIONS::const_iterator	I = std::find(m_sections.begin(),m_sections.end(),section);
	return						(I != m_sections.end());
}
