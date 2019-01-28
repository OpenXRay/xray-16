#include "StdAfx.h"
#include "UITaskWnd.h"
#include "UIMapWnd.h"
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
#include "string_table.h"
#include "Level.h"
#include "GametaskManager.h"
#include "Actor.h"
#include "xrUICore/Buttons/UICheckButton.h"

CUITaskWnd::CUITaskWnd() { hint_wnd = NULL; }
CUITaskWnd::~CUITaskWnd() { delete_data(m_pMapWnd); }
void CUITaskWnd::Init()
{
    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_TASK_XML);
    VERIFY(hint_wnd);

    CUIXmlInit::InitWindow(xml, "main_wnd", 0, this);

    m_background = UIHelper::CreateFrameWindow(xml, "background", this);

    m_cbTreasures = UIHelper::CreateCheck(xml, "filter_treasures", this);
    m_cbTreasures->SetCheck(true);
    AddCallback(m_cbTreasures, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowTreasures));
    m_bTreasuresEnabled = true;

    m_cbPrimaryObjects = UIHelper::CreateCheck(xml, "filter_primary_objects", this);
    m_cbPrimaryObjects->SetCheck(true);
    AddCallback(
        m_cbPrimaryObjects, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowPrimaryObjects));
    m_bPrimaryObjectsEnabled = true;

    m_cbSecondaryTasks = UIHelper::CreateCheck(xml, "filter_secondary_tasks", this);
    m_cbSecondaryTasks->SetCheck(true);
    AddCallback(
        m_cbSecondaryTasks, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowSecondaryTasks));
    m_bSecondaryTasksEnabled = true;

    m_cbQuestNpcs = UIHelper::CreateCheck(xml, "filter_quest_npcs", this);
    m_cbQuestNpcs->SetCheck(true);
    AddCallback(m_cbQuestNpcs, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowQuestNpcs));
    m_bQuestNpcsEnabled = true;

    m_pMapWnd = new CUIMapWnd();
    m_pMapWnd->SetAutoDelete(false);
    m_pMapWnd->hint_wnd = hint_wnd;
    m_pMapWnd->Init(PDA_TASK_XML, "map_wnd");
    AttachChild(m_pMapWnd);

    m_center_background = UIHelper::CreateStatic(xml, "center_background", this);
    m_devider = UIHelper::CreateStatic(xml, "line_devider", this);

    m_pStoryLineTaskItem = new CUITaskItem();
    m_pStoryLineTaskItem->Init(xml, "storyline_task_item");
    AttachChild(m_pStoryLineTaskItem);
    m_pStoryLineTaskItem->SetAutoDelete(true);
    AddCallback(m_pStoryLineTaskItem, WINDOW_LBUTTON_DB_CLICK,
        CUIWndCallback::void_function(this, &CUITaskWnd::OnTask1DbClicked));

    m_btn_focus = UIHelper::Create3tButton(xml, "btn_task_focus", this);
    Register(m_btn_focus);
    AddCallback(m_btn_focus, BUTTON_DOWN, CUIWndCallback::void_function(this, &CUITaskWnd::OnTask1DbClicked));

    m_BtnTaskListWnd = UIHelper::Create3tButton(xml, "btn_second_task", this);
    AddCallback(m_BtnTaskListWnd, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITaskWnd::OnShowTaskListWnd));

    m_task_wnd = new UITaskListWnd();
    m_task_wnd->SetAutoDelete(true);
    m_task_wnd->hint_wnd = hint_wnd;
    m_task_wnd->init_from_xml(xml, "second_task_wnd");
    m_pMapWnd->AttachChild(m_task_wnd);
    m_task_wnd->SetMessageTarget(this);
    m_task_wnd->Show(false);
    m_task_wnd_show = false;

    m_map_legend_wnd = new UIMapLegend();
    m_map_legend_wnd->SetAutoDelete(true);
    m_map_legend_wnd->init_from_xml(xml, "map_legend_wnd");
    m_pMapWnd->AttachChild(m_map_legend_wnd);
    m_map_legend_wnd->SetMessageTarget(this);
    m_map_legend_wnd->Show(false);
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
    else
    {
        m_pMapWnd->HideCurHint();
    }
    inherited::Update();
}

void CUITaskWnd::Draw() { inherited::Draw(); }
void CUITaskWnd::DrawHint() { m_pMapWnd->DrawHint(); }
void CUITaskWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (msg == PDA_TASK_SET_TARGET_MAP && pData)
    {
        CGameTask* task = static_cast<CGameTask*>(pData);
        TaskSetTargetMap(task);
        return;
    }
    if (msg == PDA_TASK_SHOW_MAP_SPOT && pData && m_bSecondaryTasksEnabled)
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

    inherited::SendMessage(pWnd, msg, pData);
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUITaskWnd::ReloadTaskInfo()
{
    CGameTask* t = Level().GameTaskManager().ActiveTask();
    m_pStoryLineTaskItem->InitTask(t);

    if (t && (t->m_map_object_id == u16(-1) || t->m_map_location.size() == 0))
        m_btn_focus->Show(false);
    else
        m_btn_focus->Show(true);

    Locations map_locs = Level().MapManager().Locations();
    auto b = map_locs.begin(), e = map_locs.end();
    for (; b != e; b++)
    {
        shared_str spot = b->spot_type;
        if (spot == "treasure")
            m_bTreasuresEnabled ? b->location->EnableSpot() : b->location->DisableSpot();
        else if (spot == "primary_object")
            m_bPrimaryObjectsEnabled ? b->location->EnableSpot() : b->location->DisableSpot();
        else if (spot == "secondary_task_location" || spot == "secondary_task_location_complex_timer")
            (/*b->location->SpotEnabled() && */ m_bSecondaryTasksEnabled) ? b->location->EnableSpot() :
                                                                            b->location->DisableSpot();
        else if (spot == "ui_pda2_trader_location" || spot == "ui_pda2_mechanic_location" ||
            spot == "ui_pda2_scout_location" || spot == "ui_pda2_quest_npc_location" ||
            spot == "ui_pda2_medic_location" || spot == "ui_pda2_actor_box_location" ||
            spot == "ui_pda2_actor_sleep_location")
            m_bQuestNpcsEnabled ? b->location->EnableSpot() : b->location->DisableSpot();
    }

    if (!t)
        return;

    m_actual_frame = Level().GameTaskManager().ActualFrame();

    u32 task_count = Level().GameTaskManager().GetTaskCount(eTaskStateInProgress);
    if (task_count)
    {
        u32 task_index = Level().GameTaskManager().GetTaskIndex(t, eTaskStateInProgress);
        string32 buf;
        xr_sprintf(buf, sizeof(buf), "%d / %d", task_index, task_count);
    }
    if (m_task_wnd->IsShown())
        m_task_wnd->UpdateList();
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
        m_task_wnd->Show(m_task_wnd_show);
    }
    else
    {
        //		m_task_wnd_show = false;
        m_task_wnd->Show(false);
    }
}

void CUITaskWnd::Reset() { inherited::Reset(); }
void CUITaskWnd::OnNextTaskClicked() {}
void CUITaskWnd::OnPrevTaskClicked() {}
void CUITaskWnd::OnShowTaskListWnd(CUIWindow* w, void* d)
{
    m_task_wnd_show = !m_task_wnd_show;
    m_task_wnd->Show(!m_task_wnd->IsShown());
}

void CUITaskWnd::Show_TaskListWnd(bool status)
{
    m_task_wnd->Show(status);
    m_task_wnd_show = status;
}

void CUITaskWnd::TaskSetTargetMap(CGameTask* task)
{
    if (!task || !m_bSecondaryTasksEnabled)
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
    if (!task || !m_bSecondaryTasksEnabled)
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

void CUITaskWnd::OnTask1DbClicked(CUIWindow* ui, void* d)
{
    CGameTask* task = Level().GameTaskManager().ActiveTask();
    TaskSetTargetMap(task);
}

void CUITaskWnd::ShowMapLegend(bool status) const { m_map_legend_wnd->Show(status); }
void CUITaskWnd::Switch_ShowMapLegend() const { m_map_legend_wnd->Show(!m_map_legend_wnd->IsShown()); }
void CUITaskWnd::OnShowTreasures(CUIWindow* ui, void* d)
{
    m_bTreasuresEnabled = !m_bTreasuresEnabled;
    ReloadTaskInfo();
}
void CUITaskWnd::OnShowPrimaryObjects(CUIWindow* ui, void* d)
{
    m_bPrimaryObjectsEnabled = !m_bPrimaryObjectsEnabled;
    ReloadTaskInfo();
}
void CUITaskWnd::OnShowSecondaryTasks(CUIWindow* ui, void* d)
{
    m_bSecondaryTasksEnabled = !m_bSecondaryTasksEnabled;
    ReloadTaskInfo();
}
void CUITaskWnd::OnShowQuestNpcs(CUIWindow* ui, void* d)
{
    m_bQuestNpcsEnabled = !m_bQuestNpcsEnabled;
    ReloadTaskInfo();
}
// --------------------------------------------------------------------------------------------------
CUITaskItem::CUITaskItem() : m_owner(nullptr), m_hint_wt(500), show_hint(false), show_hint_can(false) {}
CUITaskItem::~CUITaskItem() {}

void CUITaskItem::Init(CUIXml& uiXml, LPCSTR path)
{
    CUIXmlInit::InitWindow(uiXml, path, 0, this);
    m_hint_wt = uiXml.ReadAttribInt(path, 0, "hint_wt", 500);

    const auto init = [&](pcstr name, bool critical = true)
    {
        string256 buff;
        strconcat(sizeof(buff), buff, path, ":", name);
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
        if (Device.dwTimeGlobal > (m_dwFocusReceiveTime + m_hint_wt))
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
