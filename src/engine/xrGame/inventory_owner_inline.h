#pragma once

IC	CTradeParameters &CInventoryOwner::trade_parameters	() const
{
	VERIFY	(m_trade_parameters);
	return	(*m_trade_parameters);
}
