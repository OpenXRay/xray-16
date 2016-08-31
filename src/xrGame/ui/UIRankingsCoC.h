#pragma once
#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIHint;
class CUIScrollView;

class CUIRankingsCoC : public CUIWindow
{
	typedef CUIWindow inherited;
private:
	CUIScrollView*				m_parent;
	CUITextWnd*					m_name;
	CUITextWnd*					m_descr;
	CUIStatic*					m_icon;
	//CUIStatic*					m_border;
	UIHint*						m_hint;
	u8							m_index;

public:
						CUIRankingsCoC		(CUIScrollView* parent);
	virtual				~CUIRankingsCoC	();

			void		init_from_xml		(CUIXml& xml,u8 index,bool bUnique);
			void		Update				();

			void		SetName				(LPCSTR name);
			void		SetDescription		(LPCSTR desc);
			void		SetHint				(LPCSTR hint);
			void		SetIcon				(LPCSTR icon);
			void		SetFunctor			(LPCSTR func);

	virtual void		DrawHint			();
	virtual void		Reset				();

protected:
			bool		ParentHasMe			();
};
