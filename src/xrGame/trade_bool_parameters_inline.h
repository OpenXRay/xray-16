////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_bool_parameters.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade bool parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CTradeBoolParameters::CTradeBoolParameters() {}
IC void CTradeBoolParameters::clear() { m_sections.clear(); }
IC void CTradeBoolParameters::disable(const shared_str& section)
{
    VERIFY(std::find(m_sections.cbegin(), m_sections.cend(), section) == m_sections.cend());
    m_sections.push_back(section);
}

IC bool CTradeBoolParameters::disabled(const shared_str& section) const
{
    const auto I = std::find(m_sections.cbegin(), m_sections.cend(), section);
    return (I != m_sections.cend());
}
