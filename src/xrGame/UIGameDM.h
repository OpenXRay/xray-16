#pragma once

#include "UIGameMP.h"


class CUIDMPlayerList;
class CUIDMStatisticWnd;
class CUISkinSelectorWnd;
class game_cl_Deathmatch;
class CUIMoneyIndicator;
class CUIRankIndicator;
class UIVoteStatusWnd;
class CUIMapDesc;
class UITeamPanels;
class CUITextWnd;

class CUIGameDM: public UIGameMP
{
private:
	game_cl_Deathmatch *	m_game;
	typedef UIGameMP inherited;

protected:
	enum{
		flShowFragList	= (1<<1),
		fl_force_dword	= u32(-1)	};


	UITeamPanels*					m_pTeamPanels;

	CUITextWnd*						m_time_caption;
	CUITextWnd*						m_spectrmode_caption;		
	CUITextWnd*						m_spectator_caption;
	CUITextWnd*						m_pressjump_caption;
	CUITextWnd*						m_pressbuy_caption;
	CUITextWnd*						m_round_result_caption;		
	CUITextWnd*						m_force_respawn_time_caption;
	CUITextWnd*						m_demo_play_caption;
	CUITextWnd*						m_warm_up_caption;

	CUIMoneyIndicator*				m_pMoneyIndicator;
	CUIRankIndicator*				m_pRankIndicator;
	CUITextWnd*						m_pFragLimitIndicator;
	UIVoteStatusWnd*				m_voteStatusWnd;
public:
									CUIGameDM				();
	virtual 						~CUIGameDM				();

	virtual void					SetClGame				(game_cl_GameState* g);
	virtual	void					Init					(int stage);
	virtual void					UnLoad					();
	virtual void					Render					();
	virtual void	_BCL			OnFrame					();

	void							SetRank							(s16 team, u8 rank);

	virtual void					ChangeTotalMoneyIndicator		(LPCSTR newMoneyString);
	virtual void					DisplayMoneyChange				(LPCSTR deltaMoney);
	virtual void					DisplayMoneyBonus				(KillMessageStruct* bonus);
	virtual void					SetFraglimit					(int local_frags, int fraglimit);

			void					SetTimeMsgCaption				(LPCSTR str);
			void					SetSpectrModeMsgCaption			(LPCSTR str);
			void					SetSpectatorMsgCaption			(LPCSTR str);
			void					SetPressJumpMsgCaption			(LPCSTR str);
			void					SetPressBuyMsgCaption			(LPCSTR str);
			void					SetRoundResultCaption			(LPCSTR str);
			void					SetForceRespawnTimeCaption		(LPCSTR str);
			void					SetDemoPlayCaption				(LPCSTR str);
			void					SetWarmUpCaption				(LPCSTR str);

			void					SetVoteMessage					(LPCSTR str);
			void					SetVoteTimeResultMsg			(LPCSTR str);

			void					UpdateTeamPanels				();

			void					ShowFragList					(bool bShow);
			void					ShowPlayersList					(bool bShow);
};
