#pragma once

#include "UIGameCustom.h"
#include "UIGameTDM.h"

#include "ui/UIDialogWnd.h"
#include "ui/UISpawnWnd.h"

// refs 
class CUIAHuntFragList;
class CUIAHuntPlayerList;
class game_cl_ArtefactHunt;
class CUITextWnd;
class CUIMessageBoxEx;

class CUIGameAHunt: public CUIGameTDM
{
private:
	game_cl_ArtefactHunt * m_game;
	typedef CUIGameTDM inherited;

public:
	CUITextWnd*	m_pReinforcementInidcator;
	CUIMessageBoxEx*	m_pBuySpawnMsgBox;

public:
	virtual void		SetClGame				(game_cl_GameState* g);
	CUIGameAHunt								();
	virtual 			~CUIGameAHunt			();

	virtual	void		Init					(int stage);
	virtual	void		UnLoad					();
			void		SetBuyMsgCaption		(LPCSTR str);

protected:
	CUITextWnd*	m_buy_msg_caption;		
};
