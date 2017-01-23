#pragma once
#include "UIFrameWindow.h"

class CUITextWnd;

class CUIButtonHint :public CUIFrameWindow
{
	CUIWindow*			m_ownerWnd;

	CUITextWnd*			m_text;
	bool				m_enabledOnFrame;
public:
					CUIButtonHint	();
	virtual			~CUIButtonHint	();
	CUIWindow*		Owner			()	{return m_ownerWnd;}
	void			Discard			()	{m_ownerWnd=NULL;};
	void			OnRender		();
	void			Draw_			()	{m_enabledOnFrame = true;};
	void			SetHintText		(CUIWindow* w, LPCSTR text);
};

extern CUIButtonHint* g_btnHint; 
extern CUIButtonHint* g_statHint;
