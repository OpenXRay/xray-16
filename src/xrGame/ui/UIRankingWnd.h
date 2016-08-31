////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankingWnd.h
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
// 	Modified By	: Alundaio (8/22/2016)
//	Description : UI Ranking window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "UIWindow.h"
#include "UIWndCallback.h"
#include "UIAchievements.h"
#include "UIRankingsCoC.h"

class CUIStatic;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICharacterInfo;
class CUIScrollView;

class CUIRankingWnd : public CUIWindow, public CUIWndCallback
{
private:
	typedef CUIWindow	inherited;

	CUIFrameWindow*		m_background;
	CUIFrameWindow*		m_down_background;

	CUIScrollView*		m_achievements;
	CUIFrameWindow*		m_achievements_background;
	
	//Alundaio: CoC Rankings
	CUIScrollView*		m_coc_ranking;
	CUIScrollView*		m_coc_ranking_actor_view;
	CUIFrameWindow*		m_coc_ranking_background;
	//-Alundaio 
	
	DEFINE_VECTOR		(CUIAchievements*, ACHIEVES_VEC, ACHIEVES_VEC_IT);
	ACHIEVES_VEC		m_achieves_vec;
	
	//Alundaio: CoC Rankings
	DEFINE_VECTOR		(CUIRankingsCoC*, RANKINGCOC_VEC, RANKINGCOC_VEC_IT);
	RANKINGCOC_VEC		m_coc_ranking_vec;
	
	CUIRankingsCoC*		m_coc_ranking_actor;
	//-Alundaio

	enum				{ max_stat_info = 32 };
	CUITextWnd*			m_stat_caption[max_stat_info];
	CUITextWnd*			m_stat_info[max_stat_info];

	u32					m_delay;
	u32					m_previous_time;
	u32					m_stat_count;

public:
						CUIRankingWnd			();
	virtual				~CUIRankingWnd			();

	virtual void 		Show					(bool status);
	virtual void		Update					();
	virtual void		DrawHint				();
	virtual void		ResetAll				();

			void		Init					();
			void		update_info				();

protected:
			void		add_achievement			(CUIXml& xml, shared_str const& faction_id);
			void		get_statistic			();
}; // class CUIRankingWnd
