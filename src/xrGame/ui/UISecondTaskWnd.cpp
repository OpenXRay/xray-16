////////////////////////////////////////////////////////////////////////////
//	Module 		: UISecondTaskWnd.cpp
//	Created 	: 30.05.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Secondary Task Wnd class impl
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UISecondTaskWnd.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/ScrollBar/UIFixedScrollBar.h"
#include "xrUICore/Hint/UIHint.h"
#include "UITaskWnd.h"
#include "GameTaskDefs.h"
#include "GameTask.h"
#include "map_location.h"
#include "UIInventoryUtilities.h"
#include "string_table.h"
#include "Level.h"
#include "GametaskManager.h"
#include "Actor.h"

UITaskListWnd::UITaskListWnd() { hint_wnd = NULL; }
UITaskListWnd::~UITaskListWnd() {}
void UITaskListWnd::init_from_xml(CUIXml& xml, LPCSTR path)
{
    VERIFY(hint_wnd);
    CUIXmlInit::InitWindow(xml, path, 0, this);

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE tmpl_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(tmpl_root);

    m_background = UIHelper::CreateFrameWindow(xml, "background_frame", this);
    m_caption = UIHelper::CreateStatic(xml, "t_caption", this);
    //	m_counter    = UIHelper::CreateStatic( xml, "t_counter", this );
    m_bt_close = UIHelper::Create3tButton(xml, "btn_close", this);

    Register(m_bt_close);
    AddCallback(m_bt_close, BUTTON_DOWN, CUIWndCallback::void_function(this, &UITaskListWnd::OnBtnClose));

    m_list = new CUIScrollView();
    m_list->SetAutoDelete(true);
    AttachChild(m_list);
    CUIXmlInit::InitScrollView(xml, "task_list", 0, m_list);
    m_orig_h = GetHeight();

    m_list->SetWindowName("---second_task_list");
    m_list->m_sort_function = fastdelegate::MakeDelegate(this, &UITaskListWnd::SortingLessFunction);

    xml.SetLocalRoot(stored_root);
}

bool UITaskListWnd::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (inherited::OnMouseAction(x, y, mouse_action))
    {
        return true;
    }
    return true;
}

void UITaskListWnd::OnMouseScroll(float iDirection)
{
    if (iDirection == WINDOW_MOUSE_WHEEL_UP)
        m_list->ScrollBar()->TryScrollDec();
    else if (iDirection == WINDOW_MOUSE_WHEEL_DOWN)
        m_list->ScrollBar()->TryScrollInc();
}
void UITaskListWnd::Show(bool status)
{
    inherited::Show(status);
    GetMessageTarget()->SendMessage(this, PDA_TASK_HIDE_HINT, NULL);
    if (status)
        UpdateList();
}

void UITaskListWnd::OnFocusReceive()
{
    inherited::OnFocusReceive();
    GetMessageTarget()->SendMessage(this, PDA_TASK_HIDE_HINT, NULL);
}

void UITaskListWnd::OnFocusLost()
{
    inherited::OnFocusLost();
    GetMessageTarget()->SendMessage(this, PDA_TASK_HIDE_HINT, NULL);
}

void UITaskListWnd::Update()
{
    inherited::Update();
    //	UpdateCounter();
}

void UITaskListWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    GetMessageTarget()->SendMessage(pWnd, msg, pData);
    inherited::SendMessage(pWnd, msg, pData);
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void UITaskListWnd::OnBtnClose(CUIWindow* w, void* d)
{
    CUITaskWnd* wnd = smart_cast<CUITaskWnd*>(GetParent()->GetParent());
    if (wnd)
        wnd->Show_TaskListWnd(false);
    //	Show( false );
    m_bt_close->SetButtonState(CUIButton::BUTTON_NORMAL);
}

void UITaskListWnd::UpdateList()
{
    int prev_scroll_pos = m_list->GetCurrentScrollPos();

    m_list->Clear();

    u32 count_for_check = 0;
    vGameTasks& tasks = Level().GameTaskManager().GetGameTasks();
    vGameTasks::iterator itb = tasks.begin();
    vGameTasks::iterator ite = tasks.end();
    for (; itb != ite; ++itb)
    {
        CGameTask* task = (*itb).game_task;
        if (task && task->GetTaskState() == eTaskStateInProgress)
        {
            UITaskListWndItem* item = new UITaskListWndItem();
            if (item->init_task(task, this))
            {
                m_list->AddWindow(item, true);
                ++count_for_check;
            }
        }
    } // for
    m_list->SetScrollPos(prev_scroll_pos);
}

bool UITaskListWnd::SortingLessFunction(CUIWindow* left, CUIWindow* right)
{
    UITaskListWndItem* lpi = smart_cast<UITaskListWndItem*>(left);
    UITaskListWndItem* rpi = smart_cast<UITaskListWndItem*>(right);
    VERIFY(lpi && rpi);
    return (lpi->get_priority_task() > rpi->get_priority_task());
}

/*
void UITaskListWnd::UpdateCounter()
{
    u32  m_progress_task_count = Level().GameTaskManager().GetTaskCount( eTaskStateInProgress );
    CGameTask* act_task = Level().GameTaskManager().ActiveTask();
    u32 task2_index     = Level().GameTaskManager().GetTaskIndex( act_task, eTaskStateInProgress );

    string32 buf;
    xr_sprintf( buf, sizeof(buf), "%d / %d", task2_index, m_progress_task_count );
    m_counter->SetText( buf );
}
*/
// - -----------------------------------------------------------------------------------------------

UITaskListWndItem::UITaskListWndItem()
{
    m_task = NULL;

    m_color_states[0] = (u32)(-1);
    m_color_states[1] = (u32)(-1);
    m_color_states[2] = (u32)(-1);
}

UITaskListWndItem::~UITaskListWndItem() {}
IC u32 UITaskListWndItem::get_priority_task() const
{
    VERIFY(m_task);
    return m_task->m_priority;
}

bool UITaskListWndItem::init_task(CGameTask* task, UITaskListWnd* parent)
{
    VERIFY(task);
    if (!task)
    {
        return false;
    }
    m_task = task;
    SetMessageTarget(parent);

    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_TASK_XML);

    CUIXmlInit::InitWindow(xml, "second_task_wnd:task_item", 0, this);

    m_name = UIHelper::Create3tButton(xml, "second_task_wnd:task_item:name", this);
    //	m_bt_view  = UIHelper::CreateCheck(      xml, "second_task_wnd:task_item:btn_view", this );
    m_st_story = UIHelper::CreateStatic(xml, "second_task_wnd:task_item:st_story", this);
    m_bt_focus = UIHelper::Create3tButton(xml, "second_task_wnd:task_item:btn_focus", this);

    m_color_states[stt_activ] = CUIXmlInit::GetColor(xml, "second_task_wnd:task_item:activ", 0, (u32)(-1));
    m_color_states[stt_unread] = CUIXmlInit::GetColor(xml, "second_task_wnd:task_item:unread", 0, (u32)(-1));
    m_color_states[stt_read] = CUIXmlInit::GetColor(xml, "second_task_wnd:task_item:read", 0, (u32)(-1));
    update_view();
    return true;
}

void UITaskListWndItem::hide_hint()
{
    show_hint_can = false;
    show_hint = false;
    GetMessageTarget()->SendMessage(this, PDA_TASK_HIDE_HINT, NULL);
}

void UITaskListWndItem::Update()
{
    inherited::Update();
    update_view();

    if (m_task && m_name->CursorOverWindow() && show_hint_can)
    {
        if (Device.dwTimeGlobal > (m_name->FocusReceiveTime() + 700))
        {
            show_hint = true;
            GetMessageTarget()->SendMessage(this, PDA_TASK_SHOW_HINT, (void*)m_task);
            return;
        }
    }
}

void UITaskListWndItem::update_view()
{
    VERIFY(m_task);
    CMapLocation* ml = m_task->LinkedMapLocation();
    if (ml && ml->SpotEnabled())
        m_bt_focus->Show(true);
    else
        m_bt_focus->Show(false);

    if (m_task->GetTaskType() == eTaskTypeStoryline)
        m_st_story->InitTexture("ui_inGame2_PDA_icon_Primary_mission");
    else
        m_st_story->InitTexture("ui_inGame2_PDA_icon_Secondary_mission");

    m_name->TextItemControl()->SetTextST(m_task->m_Title.c_str());
    m_name->AdjustHeightToText();
    float h1 = m_name->GetWndPos().y + m_name->GetHeight() + 10.0f;
    h1 = _max(h1, GetHeight());
    SetHeight(h1);

    CGameTask* activ_task = Level().GameTaskManager().ActiveTask();

    if (m_task == activ_task)
    {
        m_name->SetStateTextColor(m_color_states[stt_activ], S_Enabled);
    }
    else if (m_task->m_read)
    {
        m_name->SetStateTextColor(m_color_states[stt_read], S_Enabled);
    }
    else
    {
        m_name->SetStateTextColor(m_color_states[stt_unread], S_Enabled);
    }
}

void UITaskListWndItem::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_bt_focus)
    {
        if (msg == BUTTON_DOWN)
        {
            GetMessageTarget()->SendMessage(this, PDA_TASK_SET_TARGET_MAP, (void*)m_task);
        }
    }
    /*
        if ( pWnd == m_bt_view )
        {
            if ( m_bt_view->GetCheck() && msg == BUTTON_CLICKED )
            {
                GetMessageTarget()->SendMessage( this, PDA_TASK_HIDE_MAP_SPOT, (void*)m_task );
    //			Msg( " HIDE task  id = %d", m_task->m_ID );
                return;
            }
            if ( !m_bt_view->GetCheck() && msg == BUTTON_CLICKED )
            {
                GetMessageTarget()->SendMessage( this, PDA_TASK_SHOW_MAP_SPOT, (void*)m_task );
    //			Msg( " show task  id = %d", m_task->m_ID );
                return;
            }
        }
    */
    if (pWnd == m_name)
    {
        if (msg == BUTTON_DOWN)
        {
            Level().GameTaskManager().SetActiveTask(m_task);
            return;
        }

        if (msg == WINDOW_LBUTTON_DB_CLICK)
        {
            GetMessageTarget()->SendMessage(this, PDA_TASK_SET_TARGET_MAP, (void*)m_task);
        }
    }

    inherited::SendMessage(pWnd, msg, pData);
}

bool UITaskListWndItem::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (inherited::OnMouseAction(x, y, mouse_action))
    {
        // return true;
    }

    switch (mouse_action)
    {
    case WINDOW_LBUTTON_DOWN:
    case WINDOW_RBUTTON_DOWN:
    case BUTTON_DOWN:
    {
        hide_hint();
        break;
    }
    } // switch

    return true;
}

void UITaskListWndItem::OnFocusReceive()
{
    inherited::OnFocusReceive();
    hide_hint();
    show_hint_can = true;
}

void UITaskListWndItem::OnFocusLost()
{
    inherited::OnFocusLost();
    hide_hint();
}
