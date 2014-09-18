////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_factor_parameters_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade factor parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeFactorParameters::CTradeFactorParameters			()
{
}

IC	void CTradeFactorParameters::clear						()
{
	m_factors.clear			();
}

IC	void CTradeFactorParameters::enable						(const shared_str &section, const CTradeFactors &factors)
{
	FACTORS::const_iterator	I = m_factors.find(section);
	VERIFY					(I == m_factors.end());
	m_factors.insert		(std::make_pair(section,factors));
}

IC	bool CTradeFactorParameters::enabled					(const shared_str &section) const
{
	FACTORS::const_iterator	I = m_factors.find(section);
	return					(I != m_factors.end());
}

IC	const CTradeFactors &CTradeFactorParameters::factors	(const shared_str &section) const
{
	FACTORS::const_iterator	I = m_factors.find(section);
	VERIFY					(I != m_factors.end());
	return					((*I).second);
}
