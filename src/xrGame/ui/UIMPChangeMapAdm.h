#pragma once

#include "UIWindow.h"
#include "UIWndCallback.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class CUIListBox;
class CUI3tButton;

class CUIMpChangeMapAdm :	public CUIWindow, public CUIWndCallback 
{
		typedef CUIWindow	inherited;
		CUIStatic*			map_pic;
		CUIStatic*			map_frame;
		CUITextWnd*			map_version;
		CUIListBox*			lst;
		CUI3tButton*		btn_ok;
public:
							CUIMpChangeMapAdm();
							~CUIMpChangeMapAdm();
				void		Init(CUIXml& xml_doc);
		virtual void		SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0);
				void 		OnBtnOk();
				void 		OnItemSelect();
				void		FillUpList();
};
