////////////////////////////////////////////////////////////////////////////
//	Module 		: UILogsWnd.h
//	Created 	: 25.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Logs (PDA) window class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_PDA_LOGS_WND_H_INCLUDED
#define UI_PDA_LOGS_WND_H_INCLUDED

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

#include "ai_space.h"
#include "xrServerEntities/alife_space.h"
#include "xrUICore/XML/xrUIXmlParser.h"

class CUIStatic;
class CUITextWnd;
class CUIXml;
class CUIProgressBar;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICharacterInfo;
class CUIScrollView;
class CUI3tButton;
class CUICheckButton;
struct GAME_NEWS_DATA;
class CUINewsItemWnd;

class CUILogsWnd final : public CUIWindow, public CUIWndCallback
{
private:
    typedef CUIWindow inherited;

    CUIFrameWindow* m_background;
    CUIFrameLineWnd* m_background2;
    CUIFrameWindow* m_center_background;
    CUIStatic* m_center_background2;

    CUITextWnd* m_center_caption;
    CUICharacterInfo* m_actor_ch_info;

    CUICheckButton* m_filter_news;
    CUICheckButton* m_filter_talk;

    CUITextWnd* m_date_caption;
    CUITextWnd* m_date;

    CUITextWnd* m_period_caption;
    CUIStatic* m_period;

    ALife::_TIME_ID m_start_game_time;
    ALife::_TIME_ID m_selected_period;

    CUI3tButton* m_prev_period;
    CUI3tButton* m_next_period;
    bool m_ctrl_press;

    CUIScrollView* m_list;
    u32 m_previous_time;
    bool m_need_reload;
    WINDOW_LIST m_items_cache;
    WINDOW_LIST m_items_ready;
    xr_vector<u32> m_news_in_queue;

    CUIWindow* CreateItem();
    CUIWindow* ItemFromCache();
    //	void				ItemToCache			(CUIWindow* w);
    CUIXml m_uiXml;

public:
    CUILogsWnd();
    ~CUILogsWnd() override;

    bool Init();

    virtual void Show(bool status);
    virtual void Update();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

    IC void UpdateNews() { m_need_reload = true; }
    void PerformWork();

    pcstr GetDebugType() override { return "CUILogsWnd"; }

protected:
    void ReLoadNews();
    void AddNewsItem(GAME_NEWS_DATA& news_data);
    ALife::_TIME_ID GetShiftPeriod(ALife::_TIME_ID datetime, int shift_day);

    void UpdateChecks(CUIWindow* w, void* d);
    void PrevPeriod(CUIWindow* w, void* d);
    void NextPeriod(CUIWindow* w, void* d);

    void on_scroll_keys(int dik);

    /*
    protected:
        void		add_faction			( CUIXml& xml, shared_str const& faction_id );
        void		clear_all_factions		();
        bool		SortingLessFunction		( CUIWindow* left, CUIWindow* right );
    */
}; // class CUILogsWnd

#endif // UI_PDA_LOGS_WND_H_INCLUDED
