//=============================================================================
//  Filename:   UIMultiTextStatic.cpp
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Статик контрол на котором можно выводить множество надписей различными
//	шрифтами, цветами и даже с анимацией
//=============================================================================

#include "stdafx.h"
#include "UIMultiTextStatic.h"
#include "../../xrEngine/CustomHUD.h"

//////////////////////////////////////////////////////////////////////////
CUIMultiTextStatic::CUIMultiTextStatic				()
{
	m_vPhrases.clear();
};

CUIMultiTextStatic::~CUIMultiTextStatic				()
{
}

CUIMultiTextStatic::SinglePhrase * CUIMultiTextStatic::AddPhrase()
{
	m_vPhrases.resize(m_vPhrases.size() + 1);
	return &m_vPhrases.back();
}

void CUIMultiTextStatic::RemovePhraseByIndex(u32 idx)
{
	Phrases_it it = m_vPhrases.begin();
	std::advance(it, idx);
	m_vPhrases.erase(it);
}

//////////////////////////////////////////////////////////////////////////

void CUIMultiTextStatic::Draw()
{
	Fvector2			p;
	GetAbsolutePos		(p);

	for (Phrases_it it = m_vPhrases.begin(); it != m_vPhrases.end(); ++it)
	{
		it->effect.Out(it->outX + p.x, it->outY + p.y, *it->str);
	}

	inherited::Draw();
}

//////////////////////////////////////////////////////////////////////////

void CUIMultiTextStatic::Update()
{
	for (Phrases_it it = m_vPhrases.begin(); it != m_vPhrases.end(); ++it)
	{
		it->effect.Update();
	}

	inherited::Update();
}

//////////////////////////////////////////////////////////////////////////

CUIMultiTextStatic::SinglePhrase * CUIMultiTextStatic::GetPhraseByIndex(u32 idx)
{
	R_ASSERT(idx < m_vPhrases.size());

	return &m_vPhrases[idx];
}



//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------/
//  SinglePhrase struct
//-----------------------------------------------------------------------------/

//////////////////////////////////////////////////////////////////////////

CUIMultiTextStatic::SPh::SPh()
	:maxWidth			(-1.0f)
{
	effect.SetStyleParams(CUITextBanner::tbsNone);
	effect.StopAnimation();
}

//////////////////////////////////////////////////////////////////////////

void CUIMultiTextStatic::SPh::SetText(const char *fmt, ...)
{
	va_list		Print;                                                                  
	string512	msg;
	xr_string	buf;

	va_start( Print , fmt );
		vsprintf_s( msg , fmt , Print );
	va_end( Print );

	buf += msg;
	msg[0] = '\n';

	str = buf.c_str();

	/*if (maxWidth > 0 && elipsisPos != CUIStatic::eepNone)
	{
		STRING s;
		CUIStatic::SetText(*str, s);

		Frect r;
		r.left		= 0;
		r.right		= maxWidth;
		r.top		= 0;
		r.bottom	= 0;

		CUIStatic::Elipsis(s, r, elipsisPos, effect.GetFont());

		str = &s.front();
	}*/
}

#include "../string_table.h"

void CUICaption::addCustomMessage(const shared_str& msg_name, float x, float y, float font_size, 
								  CGameFont *pFont, CGameFont::EAligment al, u32 color, LPCSTR def_str)
{
//	R_ASSERT2( (m_indices.find(msg_name) == m_indices.end()),"message already defined !!!" );
	R_ASSERT2( u32(-1)==findIndexOf_(msg_name),"message already defined !!!" );

	SinglePhrase * sp = AddPhrase();
	sp->outX = x;
	sp->outY = y;
	sp->effect.SetFontSize(font_size);
	sp->effect.SetFont(pFont);
	sp->effect.SetTextColor(color);
	sp->effect.SetFontAlignment(al);

	sp->str = *CStringTable().translate(def_str);
	sp->key = msg_name;

//	m_indices[msg_name] = m_vPhrases.size()-1;

}

u32 CUICaption::findIndexOf_(const shared_str& key_)
{
	for (Phrases_it it = m_vPhrases.begin(); it != m_vPhrases.end(); ++it)
	{
		if((*it).key==key_) return (u32)std::distance(m_vPhrases.begin(),it);
	}
	return u32(-1);
}

u32 CUICaption::findIndexOf(const shared_str& key_)
{
	u32 res = findIndexOf_(key_);
	R_ASSERT3(res!=u32(-1),"cannot find msg ",*key_);
	return res;
}

void CUICaption::removeCustomMessage(const shared_str& msg_name)
{
//	R_ASSERT2( (m_indices.find(msg_name) != m_indices.end()),"message not defined !!!" );
	RemovePhraseByIndex(findIndexOf(msg_name));
//	m_indices.erase(msg_name);
}

EffectParams* CUICaption::customizeMessage(const shared_str& msg_name, const CUITextBanner::TextBannerStyles styleName)
{
//	R_ASSERT2( (m_indices.find(msg_name) != m_indices.end()),"message not defined !!!" );
	
	SinglePhrase * sp = GetPhraseByIndex( findIndexOf(msg_name) );
	sp->effect.PlayAnimation();
	return sp->effect.SetStyleParams(styleName);
	
}

void CUICaption::setCaption(const shared_str& msg_name, LPCSTR message_to_out, u32 color, bool replaceColor)
{
//	R_ASSERT2( (m_indices.find(msg_name) != m_indices.end()),"message not defined !!!" );
	SinglePhrase * sp = GetPhraseByIndex(findIndexOf(msg_name));
	sp->str = *CStringTable().translate(message_to_out);

	if(replaceColor)
		sp->effect.SetTextColor(color);
}

void CUICaption::Draw()
{
	if (psHUD_Flags.is(HUD_DRAW|HUD_DRAW_RT))
	{
		inherited::Draw();
		inherited::Update();
	}	
}
