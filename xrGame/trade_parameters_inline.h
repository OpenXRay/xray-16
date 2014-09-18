////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_parameters_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeParameters::CTradeParameters						(const shared_str &section) :
	m_buy	(
		CTradeFactors(
			pSettings->r_float(section,"buy_price_factor_hostile"),
			pSettings->r_float(section,"buy_price_factor_friendly")
		)
	),
	m_sell	(
		CTradeFactors(
			pSettings->r_float(section,"sell_price_factor_hostile"),
			pSettings->r_float(section,"sell_price_factor_friendly")
		)
	)
{
}

IC	void CTradeParameters::clear							()
{
	m_buy.clear				();
	m_sell.clear			();
}

IC	CTradeParameters &CTradeParameters::instance			()
{
	if (m_instance)
		return				(*m_instance);

	m_instance				= xr_new<CTradeParameters>();
	return					(*m_instance);
}

IC	void CTradeParameters::clean							()
{
	xr_delete				(m_instance);
}

IC	CTradeParameters &default_trade_parameters				()
{
	return					(CTradeParameters::instance());
}

IC	const CTradeActionParameters &CTradeParameters::action	(action_buy) const
{
	return					(m_buy);
}

IC	const CTradeActionParameters &CTradeParameters::action	(action_sell) const
{
	return					(m_sell);
}

IC	const CTradeBoolParameters &CTradeParameters::action	(action_show) const
{
	return					(m_show);
}

IC	CTradeActionParameters &CTradeParameters::action	(action_buy)
{
	return					(m_buy);
}

IC	CTradeActionParameters &CTradeParameters::action	(action_sell)
{
	return					(m_sell);
}

IC	CTradeBoolParameters &CTradeParameters::action		(action_show)
{
	return					(m_show);
}

template <typename _action_type>
IC	bool CTradeParameters::enabled							(_action_type type, const shared_str &section) const
{
	if (action(type).disabled(section))
		return				(false);

	if (default_trade_parameters().action(type).disabled(section))
		return				(false);

	return					(true);
}

template <typename _action_type>
IC	const CTradeFactors &CTradeParameters::factors			(_action_type type, const shared_str &section) const
{
	VERIFY					(enabled(type,section));

	if (action(type).enabled(section))
		return				(action(type).factors(section));

	if (default_trade_parameters().action(type).enabled(section))
		return				(default_trade_parameters().action(type).factors(section));

	return					(action(type).default_factors());
}

template <typename _action_type>
IC	void CTradeParameters::process							(_action_type type, CInifile &ini_file, const shared_str &section)
{
	R_ASSERT2				(ini_file.section_exist(section),make_string("cannot find section %s",*section));

	CTradeActionParameters	&_action = action(type);
	_action.clear			();

	CInifile::Sect			&S = ini_file.r_section(section);
	CInifile::SectCIt		I = S.Data.begin();
	CInifile::SectCIt		E = S.Data.end();
	for ( ; I != E; ++I) {
		if (!(*I).second.size()) {
			_action.disable	((*I).first);
			continue;
		}

		string256			temp0, temp1;
		THROW3				(_GetItemCount(*(*I).second) == 2,"Invalid parameters in section",*section);
		_action.enable		(
			(*I).first,
			CTradeFactors	(
				(float)atof(_GetItem(*(*I).second,0,temp0)),
				(float)atof(_GetItem(*(*I).second,1,temp1))
			)
		);
	}
}

template <typename _action_type>
IC	void CTradeParameters::default_factors(_action_type type, const CTradeFactors &trade_factors)
{
	action(type).default_factors(trade_factors);
}
