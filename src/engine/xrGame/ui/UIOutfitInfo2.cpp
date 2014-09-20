#include "StdAfx.h"
#include "UIOutfitInfo2.h"
/*
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "UIScrollView.h"
#include "../actor.h"
#include "../CustomOutfit.h"
#include "../string_table.h"

CUIOutfitInfo2::CUIOutfitInfo2()
{
	Memory.mem_fill			(m_items, 0, sizeof(m_items));
}

CUIOutfitInfo2::~CUIOutfitInfo2()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_items[i];
		xr_delete				(_s);
	}
}

LPCSTR _imm_names []={
	"burn_immunity",
	"strike_immunity",
	"shock_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR _imm_st_names[]={
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_strike_protection",
	"ui_inv_outfit_wound_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_explosion_protection",
	"ui_inv_outfit_fire_wound_protection",
};

void CUIOutfitInfo2::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "outfit_info";

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	m_listWnd					= xr_new<CUIScrollView>(); m_listWnd->SetAutoDelete(true);
	AttachChild					(m_listWnd);
	strconcat					(sizeof(_buff),_buff, _base, ":scroll_view");
	CUIXmlInit::InitScrollView	(xml_doc, _buff, 0, m_listWnd);

	for(u32 i=ALife::eHitTypeBurn; i<= ALife::eHitTypeFireWound; ++i)
	{
		m_items[i]				= xr_new<CUIStatic>();
		CUIStatic* _s			= m_items[i];
		_s->SetAutoDelete		(false);
		strconcat				(sizeof(_buff),_buff, _base, ":static_", _imm_names[i]);
		CUIXmlInit::InitStatic	(xml_doc, _buff,	0, _s);
	}

}

void CUIOutfitInfo2::Update(CCustomOutfit* outfit)
{
	m_outfit				= outfit;

    SetItem(ALife::eHitTypeBurn,		false);
	SetItem(ALife::eHitTypeShock,		false);
	SetItem(ALife::eHitTypeStrike,		false);
	SetItem(ALife::eHitTypeWound,		false);
	SetItem(ALife::eHitTypeRadiation,	false);
	SetItem(ALife::eHitTypeTelepatic,	false);
    SetItem(ALife::eHitTypeChemicalBurn,false);
	SetItem(ALife::eHitTypeExplosion,	false);
	SetItem(ALife::eHitTypeFireWound,	false);
}

void CUIOutfitInfo2::SetItem(u32 hitType, bool force_add)
{
	string128 _buff;
	float _val_outfit	= 0.0f;
	float _val_af		= 0.0f;

	CUIStatic* _s		= m_items[hitType];

	_val_outfit			= m_outfit ? m_outfit->GetDefHitTypeProtection(ALife::EHitType(hitType)) : 1.0f;
	_val_outfit			= 1.0f - _val_outfit;


	_val_af				= smart_cast<CActor*>(Level().CurrentControlEntity())->HitArtefactsOnBelt(1.0f,ALife::EHitType(hitType));
	_val_af				= 1.0f - _val_af;

	if(fsimilar(_val_outfit, 0.0f) && fsimilar(_val_af, 0.0f) && !force_add)
	{
		if(_s->GetParent()!=NULL)
			m_listWnd->RemoveWindow(_s);
		return;
	}

//	LPCSTR			_clr_outfit, _clr_af;
	LPCSTR			_imm_name	= *CStringTable().translate(_imm_st_names[hitType]);

	int _sz			= sprintf_s	(_buff,sizeof(_buff),"%s ", _imm_name);
	_sz				+= sprintf_s	(_buff+_sz,sizeof(_buff)-_sz,"%s %+3.0f%%", (_val_outfit>0.0f)?"%c[green]":"%c[red]", _val_outfit*100.0f);

	if( !fsimilar(_val_af, 0.0f) )
	{
		_sz		+= sprintf_s	(_buff+_sz,sizeof(_buff)-_sz,"%s %+3.0f%%", (_val_af>0.0f)?"%c[green]":"%c[red]", _val_af*100.0f);
	}
	_s->SetText			(_buff);

	if(_s->GetParent()==NULL)
		m_listWnd->AddWindow(_s, false);
}
*/