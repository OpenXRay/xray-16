////////////////////////////////////////////////////////////////////////////
//	Module 		: UILogsWnd.cpp
//	Created 	: 25.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Logs (PDA) window class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UILogsWnd.h"
#include "UIXmlInit.h"
#include "UIProgressBar.h"
#include "UIFrameLineWnd.h"
#include "UIFrameWindow.h"
#include "UIScrollBar.h"
#include "UIFixedScrollBar.h"
#include "UIScrollView.h"
#include "UICheckButton.h"
#include "UIHelper.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "Actor.h"
#include "game_news.h"
#include "alife_time_manager.h"
#include "alife_registry_wrappers.h"
#include "string_table.h"
#include "UINewsItemWnd.h"
#include "xrEngine/xr_input.h"

#define PDA_LOGS_XML "pda_logs.xml"

extern u64 generate_time(u32 years, u32 months, u32 days, u32 hours, u32 minutes, u32 seconds, u32 milliseconds = 0);
extern void split_time(
    u64 time, u32& years, u32& months, u32& days, u32& hours, u32& minutes, u32& seconds, u32& milliseconds);

u64 const day2ms = u64(24 * 60 * 60 * 1000);

CUILogsWnd::CUILogsWnd()
{
    //	m_actor_ch_info			= NULL;
    m_previous_time = Device.dwTimeGlobal;
    m_selected_period = 0;
}

CUILogsWnd::~CUILogsWnd()
{
    m_list->Clear();
    delete_data(m_items_cache);
}

void CUILogsWnd::Show(bool status)
{
    m_ctrl_press = false;
    if (status)
    {
        // ALife::_TIME_ID	current_period = m_selected_period;
        //		m_actor_ch_info->InitCharacter( Actor()->object_id() );
        m_selected_period = GetShiftPeriod(Level().GetGameTime(), 0);
        //		if(current_period != m_selected_period)
        m_need_reload = true;
        Update();
    }
    // InventoryUtilities::SendInfoToActor("ui_pda_news_hide");
    inherited::Show(status);
}

void CUILogsWnd::Update()
{
    inherited::Update();
    if (m_need_reload)
        ReLoadNews();

    if (!m_items_ready.empty())
    {
        WINDOW_LIST::reverse_iterator it = m_items_ready.rbegin();
        WINDOW_LIST::reverse_iterator it_e = m_items_ready.rend();
        for (; it != it_e; ++it)
            m_list->AddWindow(*it, true);

        m_items_ready.clear();
    }
}

void CUILogsWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    inherited::SendMessage(pWnd, msg, pData);
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUILogsWnd::Init()
{
    m_uiXml.Load(CONFIG_PATH, UI_PATH, PDA_LOGS_XML);

    CUIXmlInit::InitWindow(m_uiXml, "main_wnd", 0, this);

    //	m_background		= UIHelper::CreateFrameLine( m_uiXml, "background", this );
    m_background = UIHelper::CreateFrameWindow(m_uiXml, "background", this);
    m_center_background = UIHelper::CreateFrameWindow(m_uiXml, "center_background", this);

    // m_actor_ch_info = new CUICharacterInfo();
    // m_actor_ch_info->SetAutoDelete( true );
    // AttachChild( m_actor_ch_info );
    // m_actor_ch_info->InitCharacterInfo( &m_uiXml, "actor_ch_info" );

    //	m_center_background	= UIHelper::CreateStatic( m_uiXml, "center_background", this );
    m_center_caption = UIHelper::CreateTextWnd(m_uiXml, "center_caption", this);

    string256 buf;
    xr_strcpy(buf, sizeof(buf), m_center_caption->GetText());
    xr_strcat(buf, sizeof(buf), CStringTable().translate("ui_logs_center_caption").c_str());
    m_center_caption->SetText(buf);

    CUIFixedScrollBar* tmp_scroll = new CUIFixedScrollBar();
    m_list = new CUIScrollView(tmp_scroll);
    m_list->SetAutoDelete(true);
    AttachChild(m_list);
    CUIXmlInit::InitScrollView(m_uiXml, "logs_list", 0, m_list);
    //	m_list->SetWindowName("---logs_list");
    //	m_logs_list->m_sort_function = fastdelegate::MakeDelegate( this, &CUIRankingWnd::SortingLessFunction );

    m_filter_news = UIHelper::CreateCheck(m_uiXml, "filter_news", this);
    m_filter_talk = UIHelper::CreateCheck(m_uiXml, "filter_talk", this);
    m_filter_news->SetCheck(true);
    m_filter_talk->SetCheck(true);

    //	m_date_caption = UIHelper::CreateTextWnd( m_uiXml, "date_caption", this );
    //	m_date         = UIHelper::CreateTextWnd( m_uiXml, "date", this );

    m_period_caption = UIHelper::CreateTextWnd(m_uiXml, "period_caption", this);
    m_period = UIHelper::CreateTextWnd(m_uiXml, "period", this);

    m_prev_period = UIHelper::Create3tButton(m_uiXml, "btn_prev_period", this);
    m_next_period = UIHelper::Create3tButton(m_uiXml, "btn_next_period", this);

    Register(m_filter_news);
    Register(m_filter_talk);
    Register(m_prev_period);
    Register(m_next_period);

    AddCallback(m_filter_news, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUILogsWnd::UpdateChecks));
    AddCallback(m_filter_talk, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUILogsWnd::UpdateChecks));
    AddCallback(m_prev_period, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUILogsWnd::PrevPeriod));
    AddCallback(m_next_period, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUILogsWnd::NextPeriod));

    m_start_game_time = Level().GetStartGameTime();
    m_start_game_time = GetShiftPeriod(m_start_game_time, 0);
}
void itemToCache(CUIWindow* w)
{
    w->SetAutoDelete(false);
    w->SetParent(NULL);
}

extern CActor* g_actor;

void CUILogsWnd::ReLoadNews()
{
    m_news_in_queue.clear();
    if (!g_actor)
    {
        m_need_reload = false;
        return;
    }

    LPCSTR date_str = InventoryUtilities::GetDateAsString(m_selected_period, InventoryUtilities::edpDateToDay).c_str();
    m_period->SetText(date_str);
    Fvector2 pos = m_period_caption->GetWndPos();
    pos.x = m_period->GetWndPos().x - m_period_caption->GetWidth() - m_prev_period->GetWidth() - 5.0f;
    m_period_caption->SetWndPos(pos);

    ALife::_TIME_ID end_period = GetShiftPeriod(m_selected_period, 1);

    VERIFY(m_filter_news && m_filter_talk);
    GAME_NEWS_VECTOR& news_vector = Actor()->game_news_registry->registry().objects();

    //	u32 currentNews = 0;

    bool filter_news = m_filter_news->GetCheck();
    bool filter_talk = m_filter_talk->GetCheck();

    GAME_NEWS_VECTOR::iterator ib = news_vector.begin();
    GAME_NEWS_VECTOR::iterator ie = news_vector.end();
    for (u32 idx = 0; ib != ie; ++ib, ++idx)
    {
        bool add = false;
        GAME_NEWS_DATA& gn = (*ib);
        if (gn.m_type == GAME_NEWS_DATA::eNews && filter_news)
        {
            add = true;
        }
        else if (gn.m_type == GAME_NEWS_DATA::eTalk && filter_talk)
        {
            add = true;
        }
        if (gn.receive_time < m_selected_period || end_period < gn.receive_time)
        {
            add = false;
        }

        if (add)
        {
            m_news_in_queue.push_back(idx);
            //			++currentNews;
        }
    }
    m_need_reload = false;

    if (!m_list->Empty())
    {
        m_items_cache.insert(m_items_cache.end(), m_list->Items().begin(), m_list->Items().end());
        m_list->Items().clear();

        std::for_each(m_items_cache.begin(), m_items_cache.end(), itemToCache);
    }
    PerformWork();
}

void CUILogsWnd::PerformWork()
{
    if (!m_news_in_queue.empty())
    {
        u32 count = _min(30, m_news_in_queue.size());
        for (u32 i = 0; i < count; ++i)
        {
            GAME_NEWS_VECTOR& news_vector = Actor()->game_news_registry->registry().objects();
            u32 idx = m_news_in_queue.back();
            m_news_in_queue.pop_back();
            GAME_NEWS_DATA& gn = news_vector[idx];

            AddNewsItem(gn);
        }
    }
}

CUIWindow* CUILogsWnd::CreateItem()
{
    CUINewsItemWnd* itm_res;
    itm_res = new CUINewsItemWnd();
    itm_res->Init(m_uiXml, "logs_item");
    return itm_res;
}

// void CUILogsWnd::ItemToCache(CUIWindow* w)
//{
//	CUINewsItemWnd* itm = smart_cast<CUINewsItemWnd*>(w);
//	VERIFY				(w);
//	m_items_cache.push_back(itm);
//}

CUIWindow* CUILogsWnd::ItemFromCache()
{
    CUIWindow* itm_res;
    if (m_items_cache.empty())
    {
        itm_res = CreateItem();
    }
    else
    {
        itm_res = m_items_cache.back();
        m_items_cache.pop_back();
    }
    return itm_res;
}

void CUILogsWnd::AddNewsItem(GAME_NEWS_DATA& news_data)
{
    CUIWindow* news_itm_w = ItemFromCache();
    CUINewsItemWnd* news_itm = smart_cast<CUINewsItemWnd*>(news_itm_w);
    news_itm->Setup(news_data);

    m_items_ready.push_back(news_itm);
}

void CUILogsWnd::UpdateChecks(CUIWindow* w, void* d) { m_need_reload = true; }
void CUILogsWnd::PrevPeriod(CUIWindow* w, void* d)
{
    ALife::_TIME_ID current_period = m_selected_period;
    m_selected_period = GetShiftPeriod(m_selected_period, -1);
    if (m_selected_period < m_start_game_time)
    {
        m_selected_period = m_start_game_time;
    }
    if (current_period != m_selected_period)
        m_need_reload = true;
}

void CUILogsWnd::NextPeriod(CUIWindow* w, void* d)
{
    ALife::_TIME_ID current_period = m_selected_period;
    m_selected_period = GetShiftPeriod(m_selected_period, 1); // +1
    ALife::_TIME_ID game_time = GetShiftPeriod(Level().GetGameTime(), 0);
    if (m_selected_period > game_time)
    {
        m_selected_period = game_time;
    }
    if (current_period != m_selected_period)
        m_need_reload = true;
}

ALife::_TIME_ID CUILogsWnd::GetShiftPeriod(ALife::_TIME_ID datetime, int shift_day)
{
    datetime -= (datetime % day2ms);
    datetime += (u64)shift_day * day2ms;
    return datetime;
}

bool CUILogsWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (keyboard_action == WINDOW_KEY_PRESSED)
    {
        switch (dik)
        {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_PAGEUP:
        case SDL_SCANCODE_PAGEDOWN:
        {
            on_scroll_keys(dik);
            return true;
        }
        break;
        case SDL_SCANCODE_RCTRL:
        case SDL_SCANCODE_LCTRL:
        {
            m_ctrl_press = true;
            return true;
        }
        break;
        }
    }
    m_ctrl_press = false;
    return inherited::OnKeyboardAction(dik, keyboard_action);
}

bool CUILogsWnd::OnKeyboardHold(int dik)
{
    switch (dik)
    {
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_PAGEUP:
    case SDL_SCANCODE_PAGEDOWN:
    {
        on_scroll_keys(dik);
        return true;
    }
    break;
    }
    return inherited::OnKeyboardHold(dik);
}

void CUILogsWnd::on_scroll_keys(int dik)
{
    VERIFY(m_list && m_list->ScrollBar());

    switch (dik)
    {
    case SDL_SCANCODE_UP:
    {
        int orig = m_list->ScrollBar()->GetStepSize();
        m_list->ScrollBar()->SetStepSize(1);
        m_list->ScrollBar()->TryScrollDec();
        m_list->ScrollBar()->SetStepSize(orig);
        break;
    }
    case SDL_SCANCODE_DOWN:
    {
        int orig = m_list->ScrollBar()->GetStepSize();
        m_list->ScrollBar()->SetStepSize(1);
        m_list->ScrollBar()->TryScrollInc();
        m_list->ScrollBar()->SetStepSize(orig);
        break;
    }
    case SDL_SCANCODE_PAGEUP:
    {
        if (m_ctrl_press)
        {
            m_list->ScrollToBegin();
            break;
        }
        m_list->ScrollBar()->TryScrollDec();
        break;
    }
    case SDL_SCANCODE_PAGEDOWN:
    {
        if (m_ctrl_press)
        {
            m_list->ScrollToEnd();
            break;
        }
        m_list->ScrollBar()->TryScrollInc();
        break;
    }
    } // switch
}
