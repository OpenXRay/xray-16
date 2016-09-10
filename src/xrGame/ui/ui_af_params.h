#pragma once
#include "UIWindow.h"
#include "..\..\xrServerEntities\alife_space.h"

class CInventoryItem;
class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIArtefactParamItem;

class CUIArtefactParams : public CUIWindow
{
public:
					CUIArtefactParams		();
	virtual			~CUIArtefactParams		();
			void	InitFromXml				(CUIXml& xml);
			bool	Check					(const shared_str& af_section);
			void	SetInfo					(CInventoryItem& pInvItem);

protected:
	UIArtefactParamItem*	m_immunity_item[9];
	UIArtefactParamItem*	m_restore_item[ALife::eRestoreTypeMax];
	UIArtefactParamItem*	m_additional_weight;
	UIArtefactParamItem*	m_disp_condition; //Alundaio: Show AF Condition

	CUIStatic*				m_Prop_line;

}; // class CUIArtefactParams

// -----------------------------------

class UIArtefactParamItem : public CUIWindow
{
public:
				UIArtefactParamItem	();
	virtual		~UIArtefactParamItem();
		
		void	Init				( CUIXml& xml, LPCSTR section );
		void	SetCaption			( LPCSTR name );
		void	SetValue			( float value );
	
private:
	CUIStatic*	m_caption;
	CUITextWnd*	m_value;
	float		m_magnitude;
	bool		m_sign_inverse;
	shared_str	m_unit_str;
	shared_str	m_texture_minus;
	shared_str	m_texture_plus;

}; // class UIArtefactParamItem
