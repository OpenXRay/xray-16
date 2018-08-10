////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankingWnd.h
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
// 	Modified By	: Alundaio (8/22/2016)
//	Description : UI Ranking window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
//#include "UIRankFaction.h"
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
    using inherited = CUIWindow;

    CUIFrameWindow* m_background;
    CUIFrameWindow* m_down_background;

#ifndef CALLOFCHERNOBYL_RANKING
    CUIFrameWindow* m_icon_overlay;

    CUICharacterInfo* m_actor_ch_info;

    CUITextWnd* m_money_caption;
    CUITextWnd* m_money_value;

    CUITextWnd* m_center_caption;
#endif

    CUIScrollView* m_achievements;
    CUIFrameWindow* m_achievements_background;

#ifndef CALLOFCHERNOBYL_RANKING
    CUIFrameWindow* m_monster_background;
    CUIFrameWindow* m_monster_over;
    CUIFrameWindow* m_favorite_weapon_ramka;
    CUIFrameWindow* m_favorite_weapon_over;
    CUIStatic* m_monster_icon_back;
    CUIStatic* m_monster_icon;
    CUIStatic* m_favorite_weapon_bckgrnd;
    CUIStatic* m_favorite_weapon_icon;
#else
    //Alundaio: CoC Rankings
    CUIScrollView* m_coc_ranking;
    CUIScrollView* m_coc_ranking_actor_view;
    CUIFrameWindow* m_coc_ranking_background;
    //-Alundaio
#endif

    using ACHIEVES_VEC = xr_vector<CUIAchievements*>;
    ACHIEVES_VEC m_achieves_vec;

#ifndef CALLOFCHERNOBYL_RANKING
    enum 
    {
        max_stat_info = 15
    };
#else
    //Alundaio: CoC Rankings
    using RANKINGCOC_VEC = xr_vector<CUIRankingsCoC*>;
    RANKINGCOC_VEC m_coc_ranking_vec;

    CUIRankingsCoC* m_coc_ranking_actor;
    //-Alundaio

    enum
    {
        max_stat_info = 32
    };
#endif

    CUITextWnd* m_stat_caption[max_stat_info];
    CUITextWnd* m_stat_info[max_stat_info];

    u32 m_delay;
    u32 m_previous_time;
    u32 m_stat_count;

#ifndef CALLOFCHERNOBYL_RANKING
    LPCSTR m_last_monster_icon_back;
    LPCSTR m_last_monster_icon;
    LPCSTR m_last_weapon_icon;
#endif

public:
    CUIRankingWnd();
    virtual ~CUIRankingWnd();

    virtual void Show(bool status);
    virtual void Update();
    virtual void DrawHint();
    virtual void ResetAll();

    void Init();
    void update_info();

protected:
    void add_achievement(CUIXml& xml, shared_str const& faction_id);
    void get_statistic();

#ifndef CALLOFCHERNOBYL_RANKING
    void get_best_monster();
    void get_favorite_weapon();
#endif

}; // class CUIRankingWnd
