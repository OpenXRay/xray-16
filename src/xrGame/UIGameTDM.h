#pragma once

#include "UIGameCustom.h"
#include "UIGameDM.h"

#include "ui/UIDialogWnd.h"
#include "ui/UISpawnWnd.h"

// refs 

class CUITDMFragList;
class CUITDMPlayerList;
class CUISkinSelectorWnd;
class game_cl_TeamDeathmatch;
class CUIStatic;
class CUITextWnd;
class CUISpawnWnd;

class CUIGameTDM: public CUIGameDM
{
private:
	game_cl_TeamDeathmatch * m_game;
	typedef CUIGameDM inherited;

public:
	CUISpawnWnd*		m_pUITeamSelectWnd;

protected:
	CUIStatic*			m_team1_icon;
	CUIStatic*			m_team2_icon;
	CUITextWnd*			m_team1_score;
	CUITextWnd*			m_team2_score;
	CUITextWnd*			m_buy_msg_caption;
public:
	CUIGameTDM			();
	virtual 			~CUIGameTDM				();
	virtual void		SetClGame				(game_cl_GameState* g);
	virtual	void		UnLoad					();
	virtual	void		Init					(int stage);
			void		SetScoreCaption			(int t1, int t2);			
			void		SetBuyMsgCaption		(LPCSTR str);
	virtual void		SetFraglimit			(int local_frags, int fraglimit);
	virtual void		Render					();
	virtual void _BCL	OnFrame					();
	
	virtual	bool		IR_UIOnKeyboardPress	(int dik);
	virtual bool		IR_UIOnKeyboardRelease	(int dik);
};
