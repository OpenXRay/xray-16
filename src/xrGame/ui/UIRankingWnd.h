////////////////////////////////////////////////////////////////////////////
//	Module 		: UIRankingWnd.h
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Ranking window class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "UIAchievements.h"

class CUIStatic;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICharacterInfo;
class CUIScrollView;

class CUIRankingWnd : public CUIWindow, public CUIWndCallback
{
    typedef CUIWindow inherited;

    CUIFrameWindow* m_background;
    CUIFrameWindow* m_down_background;
    CUIFrameWindow* m_icon_overlay;

    CUICharacterInfo* m_actor_ch_info;

    CUITextWnd* m_money_caption;
    CUITextWnd* m_money_value;

    CUITextWnd* m_center_caption;

    CUIScrollView* m_achievements;
    CUIFrameWindow* m_achievements_background;
    CUIFrameWindow* m_monster_background;
    CUIFrameWindow* m_monster_over;
    CUIFrameWindow* m_favorite_weapon_ramka;
    CUIFrameWindow* m_favorite_weapon_over;
    CUIStatic* m_monster_icon_back;
    CUIStatic* m_monster_icon;
    CUIStatic* m_favorite_weapon_bckgrnd;
    CUIStatic* m_favorite_weapon_icon;

    using ACHIEVES_VEC = xr_vector<CUIAchievements*>;
    ACHIEVES_VEC m_achieves_vec;

    enum
    {
        max_stat_info = 15
    };
    CUITextWnd* m_stat_caption[max_stat_info];
    CUITextWnd* m_stat_info[max_stat_info];

    u32 m_delay;
    u32 m_previous_time;
    u32 m_stat_count;
    LPCSTR m_last_monster_icon_back;
    LPCSTR m_last_monster_icon;
    LPCSTR m_last_weapon_icon;

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
    void get_best_monster();
    void get_favorite_weapon();

}; // class CUIRankingWnd
