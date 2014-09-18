#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUITextWnd;
class CUI3tButton;
class CUIListBox;
class CUIFrameWindow;



class CUIVote : public CUIDialogWnd 
{
public:

					CUIVote		();
			void 	Init		();
	virtual void 	Update		();
	virtual void 	SendMessage	(CUIWindow* pWnd, s16 msg, void* pData = 0);
			void 	OnBtnYes	();
			void 	OnBtnNo		();
			void 	OnBtnCancel	();
			void 	SetVoting	(LPCSTR txt);
protected:
	CUITextWnd*		msg;
	CUITextWnd*		cap[3];
	CUIFrameWindow* frame[3];
	CUIListBox*		list[3];

	CUI3tButton*	btn_yes;
	CUI3tButton*	btn_no;
	CUI3tButton*	btn_cancel;
	CUIStatic*		bkgrnd;
	u32				m_prev_upd_time;
};