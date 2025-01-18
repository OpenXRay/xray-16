#include "StdAfx.h"
#include "UITaskWnd.h"
#include "UIMapWnd.h"
#include "UIMapFilters.h"
#include "Common/object_broker.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UISecondTaskWnd.h"
#include "UIMapLegend.h"
#include "UIHelper.h"
#include "xrUICore/Hint/UIHint.h"
#include "GameTask.h"
#include "map_location.h"
#include "map_location_defs.h"
#include "map_manager.h"
#include "UIInventoryUtilities.h"
#include "Level.h"
#include "GametaskManager.h"
#include "Actor.h"
#include "xrUICore/Buttons/UICheckButton.h"

CUITaskWnd::CUITaskWnd(UIHint* hint) : CUIWindow("CUITaskWnd"), hint_wnd(hint) {}

CUITaskWnd::~CUITaskWnd() { delete_data(m_pMapWnd); }

bool CUITaskWnd::Init()
{
    CUIXml xml;
    if (!xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_TASK_XML, false))
        return false;

    VERIFY(hint_wnd);

    CUIXmlInit::InitWindow(xml, "main_wnd", 0, this);

    std::ignore = UIHelper::CreateFrameWindow(xml, "background", this, false);
    std::ignore = UIHelper::CreateFrameLine(xml, "background", this, false);

    std::ignore = UIHelper::CreateFrameLine(xml, "task_split", this, false);

    m_filters = xr_new<CUIMapFilters>();
    if (!m_filters->Init(xml))
        xr_delete(m_filters);
    else
    {
        AttachChild(m_filters);
        m_filters->SetMessageTarget(this);
    }

    m_pMapWnd = xr_new<CUIMapWnd>(hint_wnd);
    m_pMapWnd->SetAutoDelete(false);
    m_pMapWnd->Init(PDA_TASK_XML, "map_wnd");
    AttachChild(m_pMapWnd);

    m_center_background = UIHelper::CreateStatic(xml, "center_background", this);
    std::ignore = UIHelper::CreateStatic(xml, "line_devider", this, false);

    m_pStoryLineTaskItem = xr_new<CUITaskItem>();
    m_pStoryLineTaskItem->Init(xml, "storyline_task_item");
    AttachChild(m_pStoryLineTaskItem);
    m_pStoryLineTaskItem->SetAutoDelete(true);
    AddCallback(m_pStoryLineTaskItem, WINDOW_LBUTTON_DB_CLICK,
        CUIWndCallback::void_function(this, &CUITaskWnd::OnTask1DbClicked));

    if (xml.NavigateToNode("secondary_task_item")) // XXX: replace with UIHelper
    {
        Level().GameTaskManager().AllowMultipleTask(true);
        m_pSecondaryTaskItem = xr_new<CUITaskItem>();
        m_pSecondaryTaskItem->Init(xml, "secondary_task_item");
        AttachChild(m_pSecondaryTaskItem);
        m_pSecondaryTaskItem->SetAutoDelete(true);
        AddCallback(m_pSecondaryTaskItem, WINDOW_LBUTTON_DB_CLICK, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask2DbClicked));
    }

    m_btn_focus = UIHelper::Create3tButton(xml, "btn_task_focus", this);
    Register(m_btn_focus);
    AddCallback(m_btn_focus, BUTTON_DOWN, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask1DbClicked));
    // XXX: 3tButtonEx
    //m_btn_focus->set_hint_wnd(hint_wnd);

    m_btn_focus2 = UIHelper::Create3tButton(xml, "btn_task_focus2", this, false);
    if (m_btn_focus2)
    {
        Register(m_btn_focus2);
        AddCallback(m_btn_focus2, BUTTON_DOWN, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask2DbClicked));
        //m_btn_focus2->set_hint_wnd(hint_wnd);
    }

    auto* btnTaskListWnd = UIHelper::Create3tButton(xml, "btn_second_task", this);
    AddCallback(btnTaskListWnd, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowTaskListWnd));
    btnTaskListWnd->SetAccelerator(kSCORES, false, 2);
    btnTaskListWnd->SetAccelerator(kUI_ACTION_1, false, 3);

    m_second_task_index = UIHelper::CreateStatic(xml, "second_task_index", this, false);

    m_task_wnd = xr_new<UITaskListWnd>();
    m_task_wnd->SetAutoDelete(true);
    m_task_wnd->hint_wnd = hint_wnd;
    m_task_wnd->init_from_xml(xml, "second_task_wnd");
    m_task_wnd->ShowOnlySecondaryTasks(m_pSecondaryTaskItem != nullptr);

    m_pMapWnd->AttachChild(m_task_wnd);
    m_task_wnd->SetMessageTarget(this);
    m_task_wnd->Show(false);

    m_map_legend_wnd = xr_new<UIMapLegend>();
    m_map_legend_wnd->SetAutoDelete(true);
    m_map_legend_wnd->init_from_xml(xml, "map_legend_wnd");
    m_pMapWnd->AttachChild(m_map_legend_wnd);
    m_map_legend_wnd->SetMessageTarget(this);
    m_map_legend_wnd->Show(false);

    return true;
}

void CUITaskWnd::Update()
{
    if (Level().GameTaskManager().ActualFrame() != m_actual_frame)
    {
        ReloadTaskInfo();
    }

    if (m_pStoryLineTaskItem->show_hint && m_pStoryLineTaskItem->OwnerTask())
    {
        m_pMapWnd->ShowHintTask(m_pStoryLineTaskItem->OwnerTask(), m_pStoryLineTaskItem);
    }
    else if (m_pSecondaryTaskItem && m_pSecondaryTaskItem->show_hint && m_pSecondaryTaskItem->OwnerTask())
    {
        m_pStoryLineTaskItem->show_hint = false;
        m_pMapWnd->ShowHintTask(m_pSecondaryTaskItem->OwnerTask(), m_pSecondaryTaskItem);
    }
    else
    {
        m_pMapWnd->HideCurHint();
    }
    inherited::Update();
}

void CUITaskWnd::Draw() { inherited::Draw(); }
void CUITaskWnd::DrawHint() { m_pMapWnd->DrawHint(); }

bool CUITaskWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (m_pKeyboardCapturer && pInput->IsCurrentInputTypeController())
        return m_pKeyboardCapturer->OnKeyboardAction(dik, keyboard_action);
    return inherited::OnKeyboardAction(dik, keyboard_action);
}

bool CUITaskWnd::OnControllerAction(int axis, float x, float y, EUIMessages controller_action)
{
    if (m_pKeyboardCapturer && pInput->IsCurrentInputTypeController())
        return m_pKeyboardCapturer->OnControllerAction(axis, x, y, controller_action);
    return inherited::OnControllerAction(axis, x, y, controller_action);
}

void CUITaskWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (msg == WINDOW_KEYBOARD_CAPTURE_LOST && pWnd == this)
    {
        if ((m_filters && pData == m_filters) || pData == m_task_wnd)
        {
            if (pInput->IsCurrentInputTypeController())
                UI().GetUICursor().WarpToWindow(m_pMapWnd, true);
            return;
        }
    }
    if (msg == PDA_TASK_SET_TARGET_MAP && pData)
    {
        CGameTask* task = static_cast<CGameTask*>(pData);
        TaskSetTargetMap(task);
        return;
    }
    if (msg == PDA_TASK_SHOW_MAP_SPOT && pData && IsSecondaryTasksEnabled())
    {
        CGameTask* task = static_cast<CGameTask*>(pData);
        TaskShowMapSpot(task, true);
        return;
    }
    if (msg == PDA_TASK_HIDE_MAP_SPOT && pData)
    {
        CGameTask* task = static_cast<CGameTask*>(pData);
        TaskShowMapSpot(task, false);
        return;
    }
    if (msg == PDA_TASK_SHOW_HINT && pData)
    {
        CGameTask* task = static_cast<CGameTask*>(pData);
        m_pMapWnd->ShowHintTask(task, pWnd);
        return;
    }
    if (msg == PDA_TASK_HIDE_HINT)
    {
        m_pMapWnd->HideCurHint();
        return;
    }
    if (msg == PDA_TASK_RELOAD_FILTERS)
    {
        ReloadTaskInfo();
        return;
    }

    inherited::SendMessage(pWnd, msg, pData);
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUITaskWnd::ReloadTaskInfo()
{
    CGameTask* storyTask = Level().GameTaskManager().ActiveTask(eTaskTypeStoryline);
    m_pStoryLineTaskItem->InitTask(storyTask);

    CGameTask* additionalTask = nullptr;
    if (m_pSecondaryTaskItem)
    {
        additionalTask = Level().GameTaskManager().ActiveTask(eTaskTypeAdditional);
        m_pSecondaryTaskItem->InitTask(additionalTask);
    }

    if (!storyTask || (storyTask->m_map_object_id == u16(-1) || storyTask->m_map_location.empty()))
        m_btn_focus->Show(false);
    else
        m_btn_focus->Show(true);

    if (m_btn_focus2)
    {
        if (!additionalTask || (additionalTask->m_map_object_id == u16(-1) || additionalTask->m_map_location.empty()))
            m_btn_focus2->Show(false);
        else
            m_btn_focus2->Show(true);
    }

    vLocations map_locs = Level().MapManager().Locations();
    auto b = map_locs.begin(), e = map_locs.end();
    for (; b != e; ++b)
    {
        shared_str spot = b->spot_type;
        if (strstr(spot.c_str(), "treasure"))
            IsTreasuresEnabled() ? b->location->EnableSpot() : b->location->DisableSpot();
        else if (spot == "primary_object")
            IsPrimaryObjectsEnabled() ? b->location->EnableSpot() : b->location->DisableSpot();
        else if (spot == "secondary_task_location" || spot == "secondary_task_location_complex_timer")
            (/*b->location->SpotEnabled() && */ IsSecondaryTasksEnabled()) ? b->location->EnableSpot() :
                                                                            b->location->DisableSpot();
        else if (spot == "ui_pda2_trader_location" || spot == "ui_pda2_mechanic_location" ||
            spot == "ui_pda2_scout_location" || spot == "ui_pda2_quest_npc_location" ||
            spot == "ui_pda2_medic_location" || spot == "ui_pda2_actor_box_location" ||
            spot == "ui_pda2_actor_sleep_location")
            IsQuestNpcsEnabled() ? b->location->EnableSpot() : b->location->DisableSpot();
    }

    if (storyTask || additionalTask)
    {
        m_actual_frame = Level().GameTaskManager().ActualFrame();
        if (m_task_wnd->IsShown())
            m_task_wnd->UpdateList();
    }

    if (!m_second_task_index)
        return;

    if (storyTask && !additionalTask)
    {
        const auto task_count = Level().GameTaskManager().GetTaskCount(eTaskStateInProgress, eTaskTypeStoryline);
        if (task_count)
        {
            const auto task_index = Level().GameTaskManager().GetTaskIndex(storyTask, eTaskStateInProgress, eTaskTypeStoryline);
            string32 buf;
            xr_sprintf(buf, sizeof(buf), "%d / %d", task_index, task_count);

            m_second_task_index->SetVisible(true);
            m_second_task_index->SetText(buf);
        }
        else
        {
            m_second_task_index->SetVisible(false);
            m_second_task_index->SetText("");
        }
    }

    if (additionalTask)
    {
        const auto task2_count = Level().GameTaskManager().GetTaskCount(eTaskStateInProgress, eTaskTypeAdditional);

        if (task2_count)
        {
            const auto task2_index = Level().GameTaskManager().GetTaskIndex(additionalTask, eTaskStateInProgress, eTaskTypeAdditional);
            string32 buf;
            xr_sprintf(buf, sizeof(buf), "%d / %d", task2_index, task2_count);

            m_second_task_index->SetVisible(true);
            m_second_task_index->SetText(buf);
        }
        else
        {
            m_second_task_index->SetVisible(false);
            m_second_task_index->SetText("");
        }
    }
}

void CUITaskWnd::Show(bool status)
{
    inherited::Show(status);
    m_pMapWnd->Show(status);
    m_pMapWnd->HideCurHint();
    m_map_legend_wnd->Show(false);
    if (status)
    {
        ReloadTaskInfo();
    }
}

void CUITaskWnd::OnShowTaskListWnd(CUIWindow* w, void* d) const
{
    m_task_wnd->Show(!m_task_wnd->IsShown());
}

void CUITaskWnd::Show_TaskListWnd(bool status) const
{
    m_task_wnd->Show(status);
}

bool CUITaskWnd::IsTreasuresEnabled() const { return m_filters && m_filters->IsFilterEnabled(CUIMapFilters::Treasures); }
bool CUITaskWnd::IsQuestNpcsEnabled() const { return m_filters && m_filters->IsFilterEnabled(CUIMapFilters::QuestNpcs); }
bool CUITaskWnd::IsSecondaryTasksEnabled() const { return m_filters && m_filters->IsFilterEnabled(CUIMapFilters::SecondaryTasks); }
bool CUITaskWnd::IsPrimaryObjectsEnabled() const { return m_filters && m_filters->IsFilterEnabled(CUIMapFilters::PrimaryObjects); }

void CUITaskWnd::TreasuresEnabled(bool enable)
{
    if (m_filters)
        m_filters->SetFilterEnabled(CUIMapFilters::Treasures, enable);
}
void CUITaskWnd::QuestNpcsEnabled(bool enable)
{
    if (m_filters)
        m_filters->SetFilterEnabled(CUIMapFilters::QuestNpcs, enable);
}
void CUITaskWnd::SecondaryTasksEnabled(bool enable)
{
    if (m_filters)
        m_filters->SetFilterEnabled(CUIMapFilters::SecondaryTasks, enable);
}
void CUITaskWnd::PrimaryObjectsEnabled(bool enable)
{
    if (m_filters)
        m_filters->SetFilterEnabled(CUIMapFilters::PrimaryObjects, enable);
}

bool CUITaskWnd::IsUsingCursorRightNow() const
{
    return true;
}

void CUITaskWnd::TaskSetTargetMap(CGameTask* task) const
{
    if (!task || !IsSecondaryTasksEnabled())
    {
        return;
    }

    TaskShowMapSpot(task, true);
    CMapLocation* ml = task->LinkedMapLocation();
    if (ml && ml->SpotEnabled())
    {
        ml->CalcPosition();
        m_pMapWnd->SetTargetMap(ml->GetLevelName(), ml->GetPosition(), true);
    }
}

void CUITaskWnd::TaskShowMapSpot(CGameTask* task, bool show) const
{
    if (!task || !IsSecondaryTasksEnabled())
    {
        return;
    }

    CMapLocation* ml = task->LinkedMapLocation();
    if (ml)
    {
        if (show)
        {
            ml->EnableSpot();
            ml->CalcPosition();
            m_pMapWnd->SetTargetMap(ml->GetLevelName(), ml->GetPosition(), true);
        }
        else
        {
            ml->DisableSpot();
        }
    }
}

void CUITaskWnd::OnTask1DbClicked(CUIWindow*, void*)
{
    CGameTask* task = Level().GameTaskManager().ActiveTask(eTaskTypeStoryline);
    TaskSetTargetMap(task);
}

void CUITaskWnd::OnTask2DbClicked(CUIWindow*, void*)
{
    CGameTask* task = Level().GameTaskManager().ActiveTask(eTaskTypeAdditional);
    TaskSetTargetMap(task);
}

void CUITaskWnd::Switch_ShowMapLegend() const { m_map_legend_wnd->Show(!m_map_legend_wnd->IsShown()); }

// --------------------------------------------------------------------------------------------------
CUITaskItem::CUITaskItem() : CUIWindow("CUITaskItem"), m_hint_wt(500) {}

void CUITaskItem::Init(CUIXml& uiXml, LPCSTR path)
{
    CUIXmlInit::InitWindow(uiXml, path, 0, this);
    m_hint_wt = uiXml.ReadAttribInt(path, 0, "hint_wt", 500);

    const auto init = [&](pcstr name, bool critical = true)
    {
        string256 buff;
        strconcat(buff, path, ":", name);
        m_info[name] = UIHelper::CreateStatic(uiXml, buff, this, critical);
    };

    init("t_icon", false);
    init("t_icon_over", false);
    init("t_caption");

    // If icon exist but icon_over doesn't
    // then just use icon for both cases
    if (!m_info["t_icon_over"])
        m_info["t_icon_over"] = m_info["t_icon"];

    show_hint_can = false;
    show_hint = false;
}

void CUITaskItem::InitTask(CGameTask* task)
{
    m_owner = task;
    CUIStatic* S = m_info["t_icon"];
    if (S)
    {
        if (task)
        {
            S->InitTexture(task->m_icon_texture_name.c_str());
            S->SetStretchTexture(true);
            m_info["t_icon_over"]->Show(true);
        }
        else
        {
            S->TextureOff();
            m_info["t_icon_over"]->Show(false);
        }
    }

    S = m_info["t_caption"];
    S->TextItemControl()->SetTextST((task) ? task->m_Title.c_str() : "");
}

void CUITaskItem::OnFocusReceive()
{
    inherited::OnFocusReceive();
    show_hint_can = true;
    show_hint = false;
}

void CUITaskItem::OnFocusLost()
{
    inherited::OnFocusLost();
    show_hint_can = false;
    show_hint = false;
}

void CUITaskItem::Update()
{
    inherited::Update();
    if (m_owner && m_bCursorOverWindow && show_hint_can)
    {
        if (Device.dwTimeGlobal > (m_dwFocusReceiveTime + m_hint_wt * Device.time_factor()))
        {
            show_hint = true;
            return;
        }
    }
}

void CUITaskItem::OnMouseScroll(float iDirection) {}
bool CUITaskItem::OnMouseAction(float x, float y, EUIMessages mouse_action)
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
        show_hint_can = false;
        show_hint = false;
        break;
    } // switch

    return true;
}

void CUITaskItem::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { inherited::SendMessage(pWnd, msg, pData); }
