////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankFraction.h
//	Created 	: 22.01.2008
//  Modified 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Rank Fraction info window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "UIWindow.h"
#include "FractionState.h"

class CUIXml;
class CUIStatic;
class CUIProgressBar;
class CActor;

class CUIRankFraction : public CUIWindow
{
private:
	FractionState	m_fraction_state;

	CUIStatic*		m_sn;
	CUIStatic*		m_name;
	CUIStatic*		m_icon;
	CUIStatic*		m_location_static;
	CUIStatic*		m_location_value;
	CUIStatic*		m_power_static;
	CUIStatic*		m_power_value;
	
	CUIProgressBar*		m_relation_minus;
	CUIProgressBar*		m_relation_center_minus;
	CUIProgressBar*		m_relation_center_plus;
	CUIProgressBar*		m_relation_plus;
	
	CUIStatic*		m_origin_static;
	CUIStatic*		m_border_minus;
	CUIStatic*		m_border_plus;
	CUIStatic*		m_enemy_static;
	CUIStatic*		m_frined_static;

public:
			CUIRankFraction		();
			CUIRankFraction		( shared_str const& fraction_id );
	virtual	~CUIRankFraction	();

	void	init_from_xml		( CUIXml& xml );
	void	update_info			( u8 sn );
	float	get_fraction_power	();

}; // class CUIRankFraction
