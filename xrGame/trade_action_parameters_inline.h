////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_action_parameters_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade action parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeActionParameters::CTradeActionParameters				(const CTradeFactors &default_factors)
{
	m_default			= default_factors;
}

IC	void CTradeActionParameters::clear							()
{
	m_enabled.clear		();
	m_disabled.clear	();
}

IC	void CTradeActionParameters::enable							(const shared_str &section, const CTradeFactors &trade_factors)
{
	m_enabled.enable	(section,trade_factors);
}

IC	void CTradeActionParameters::disable						(const shared_str &section)
{
	m_disabled.disable	(section);
}

IC	bool CTradeActionParameters::enabled						(const shared_str &section) const
{
	return				(m_enabled.enabled(section));
}

IC	bool CTradeActionParameters::disabled						(const shared_str &section) const
{
	return				(m_disabled.disabled(section));
}

IC	const CTradeFactors &CTradeActionParameters::factors		(const shared_str &section) const
{
	return				(m_enabled.factors(section));
}

IC	const CTradeFactors &CTradeActionParameters::default_factors() const
{
	return				(m_default);
}

IC	void CTradeActionParameters::default_factors				(const CTradeFactors &trade_factors)
{
	m_default			 =trade_factors;
}
