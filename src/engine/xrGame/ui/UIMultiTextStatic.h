//=============================================================================
//  Filename:   UIMultiTextStatic.cpp
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Статик контрол на котором можно выводить множество надписей различными
//	шрифтами, цветами и даже с анимацией
//=============================================================================

#ifndef UI_MULTITEXT_STATIC_H_
#define UI_MULTITEXT_STATIC_H_

#pragma once

#include "UITextBanner.h"

class CUIMultiTextStatic: public CUIStatic
{
	typedef CUIStatic inherited;
public:
	typedef struct SPh
	{
		float						outX;
		float						outY;
		float						maxWidth;
		CUITextBanner				effect;
		shared_str					str;

		void						SetText	(const char *fmt, ...);

		// Ctor		
		SPh							();
		shared_str					key;
	} SinglePhrase;

	typedef xr_vector<SinglePhrase>	Phrases;
	typedef Phrases::iterator		Phrases_it;
protected:
	Phrases							m_vPhrases;
public:

					CUIMultiTextStatic				();
	virtual			~CUIMultiTextStatic				();

	virtual void Draw();
	virtual void Update();
	// Добавить надпись
	// Return:	Указатель на добавленную запись
	SinglePhrase * AddPhrase();
	// Получить запись по номеру
	SinglePhrase * GetPhraseByIndex(u32 idx);
	void			RemovePhraseByIndex(u32 idx);
};

class CUICaption :protected CUIMultiTextStatic
{
	typedef CUIMultiTextStatic inherited;

	u32					findIndexOf(const shared_str& key);
	u32					findIndexOf_(const shared_str& key);
public:
	virtual void		Draw();
	void				addCustomMessage(const shared_str& msg_name, float x, float y, float font_size, CGameFont *pFont, CGameFont::EAligment al, u32 color, LPCSTR def_str="");
	EffectParams*		customizeMessage(const shared_str& msg_name, const CUITextBanner::TextBannerStyles styleName);
	void				setCaption(const shared_str& msg_name, LPCSTR message_to_out, u32 color=0, bool replaceColor=false);
	void				removeCustomMessage(const shared_str& msg_name);
};
#endif	//UI_MULTITEXT_STATIC_H_