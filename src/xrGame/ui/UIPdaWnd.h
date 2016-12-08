#pragma once

#include "UIDialogWnd.h"
#include "../encyclopedia_article_defs.h"

class CInventoryOwner;
class CUIFrameLineWnd;
class CUI3tButton;
class CUITabControl;
class CUIStatic;
class CUITextWnd;
class CUIXml;
class CUIFrameWindow;
class UIHint;

class CUITaskWnd;
//-class CUIFactionWarWnd;
class CUIRankingWnd;
class CUILogsWnd;
class CUIAnimatedStatic;
class UIHint;

class CMapSpot;


class CUIPdaWnd: public CUIDialogWnd
{
	typedef CUIDialogWnd	inherited;
protected:
	CUITabControl*			UITabControl;
	CUI3tButton*			m_btn_close;

	CUIStatic*				UIMainPdaFrame;
	CUIStatic*				UINoice;
	
	CUITextWnd*				m_caption;
	shared_str				m_caption_const;
//	CUIAnimatedStatic*		m_anim_static;
	CUITextWnd*				m_clock;

	// Текущий активный диалог
	CUIWindow*				m_pActiveDialog;
	shared_str				m_sActiveSection;

	UIHint*					m_hint_wnd;

public:
	CUITaskWnd*				pUITaskWnd;
//-	CUIFactionWarWnd*		pUIFactionWarWnd;
	CUIRankingWnd*			pUIRankingWnd;
	CUILogsWnd*				pUILogsWnd;
	
	CMapSpot*				pSelectedMapSpot;

	virtual void			Reset				();

public:
							CUIPdaWnd			();
	virtual					~CUIPdaWnd			();

	virtual void 			Init				();

	virtual void 			SendMessage			(CUIWindow* pWnd, s16 msg, void* pData = NULL);

	virtual void 			Draw				();
	virtual void 			Update				();
	virtual void 			Show				(bool status);
	virtual bool			OnMouseAction				(float x, float y, EUIMessages mouse_action) {CUIDialogWnd::OnMouseAction(x,y,mouse_action);return true;} //always true because StopAnyMove() == false
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);
		
			UIHint*			get_hint_wnd		() const { return m_hint_wnd; }
			void			DrawHint			();

			void			SetActiveCaption	();
			void			SetCaption			(LPCSTR text);
			void			Show_SecondTaskWnd	(bool status);
			void			Show_MapLegendWnd	(bool status);

			void 			SetActiveDialog		(CUIWindow* pUI) 	{ m_pActiveDialog = pUI; };
			CUIWindow*		GetActiveDialog		() 					{return m_pActiveDialog;};
			LPCSTR			GetActiveSection	()					{return m_sActiveSection.c_str();};
			
			void			SetActiveSubdialog			(const shared_str& section);
			void			SetActiveSubdialog_script(LPCSTR section)				{ SetActiveSubdialog((const shared_str&)section); };
	virtual bool			StopAnyMove			(){return false;}

			void			UpdatePda			();
			void			UpdateRankingWnd	();
			DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CUIPdaWnd)
#undef script_type_list
#define script_type_list save_type_list(CUIPdaWnd)