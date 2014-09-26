#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUI3tButton;
class CUIFrameWindow;
class CUIListBox;
class CUISpinNum;
class CUIXml;
struct game_PlayerState;

class CUIKickPlayer : public CUIDialogWnd 
{
public:
					CUIKickPlayer	();

			void	InitKick		(CUIXml& xml_doc);
			void	InitBan			(CUIXml& xml_doc);

	virtual bool	OnKeyboardAction		(int dik, EUIMessages keyboard_action);
	virtual void	SendMessage		(CUIWindow* pWnd, s16 msg, void* pData = 0);
	virtual void	Update			();

	void			OnBtnOk			();
	void			OnBtnCancel		();

protected:
	typedef enum{MODE_KICK, MODE_BAN } E_MODE;

	void			Init_internal		(CUIXml& xml_doc);

	E_MODE								mode;

	CUIStatic*							bkgrnd;
	CUIStatic*							header;
	CUIFrameWindow* 					lst_back;
	CUIListBox*							m_ui_players_list;
	CUISpinNum*							m_spin_ban_sec;
	CUIStatic*							m_ban_sec_label;

	CUI3tButton*						btn_ok;
	CUI3tButton*						btn_cancel;

	u32									m_prev_upd_time;

	shared_str							m_selected_item_text;
	xr_vector<game_PlayerState*>		m_current_set;
};