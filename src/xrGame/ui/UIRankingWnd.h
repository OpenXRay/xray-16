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

class CUIStatic;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICharacterInfo;
class CUIScrollView;
class CUIAchievements;
class CUIRankingsCoC;

class CUIRankingWnd final : public CUIWindow, public CUIWndCallback
{
    using inherited = CUIWindow;

    CUICharacterInfo* m_actor_ch_info{};

    CUIStatic* m_money_value{};

    CUIScrollView* m_factions_list{};

    CUIScrollView* m_achievements{};
    CUIStatic* m_monster_icon_back{};
    CUIStatic* m_monster_icon{};
    CUIStatic* m_favorite_weapon_icon{};

    xr_vector<CUIAchievements*> m_achieves_vec;

    //Alundaio: CoC Rankings
    xr_vector<CUIRankingsCoC*> m_coc_ranking_vec;
    CUIRankingsCoC* m_coc_ranking_actor;
    //-Alundaio

    xr_vector<CUIStatic*> m_stat_info;

    u32 m_delay;
    u32 m_previous_time;
    LPCSTR m_last_monster_icon_back;
    LPCSTR m_last_monster_icon;
    LPCSTR m_last_weapon_icon;

public:
    CUIRankingWnd();
    ~CUIRankingWnd() override;

    virtual void Show(bool status);
    virtual void Update();
    virtual void DrawHint();
    virtual void ResetAll();

    bool Init();
    void update_info();

    pcstr GetDebugType() override { return "CUIRankingWnd"; }

protected:
    void add_faction(CUIXml& xml, shared_str const& faction_id);
    void clear_all_factions();
    bool SortingLessFunction(CUIWindow* left, CUIWindow* right);

    void add_achievement(CUIXml& xml, shared_str const& achiev_id);
    void get_statistic() const;
    void get_best_monster();
    void get_favorite_weapon();
}; // class CUIRankingWnd
