////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_bool_parameters.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade bool parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

class CTradeBoolParameters {
public:
	typedef xr_vector<shared_str>	SECTIONS;

private:
	SECTIONS	m_sections;

public:
	IC			CTradeBoolParameters	();
	IC	void	clear					();
	IC	void	disable					(const shared_str &section);
	IC	bool	disabled				(const shared_str &section) const;
};

#include "trade_bool_parameters_inline.h"