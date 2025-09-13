#include "StdAfx.h"
#include "UIPdaWnd.h"
#include "PDA.h"
#include "GameTask.h"
#include "Actor.h"
#include "alife_registry_wrappers.h"
#include "GameTaskManager.h"
#include "UIEventsWnd.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIInventoryUtilities.h"

#include "Level.h"
#include "UIGameCustom.h"

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "UIMapWnd.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "Common/object_broker.h"
#include "UIMessagesWindow.h"
#include "UIMainIngameWnd.h"
#include "xrUICore/TabControl/UITabButton.h"
#include "xrUICore/Static/UIAnimatedStatic.h"

#include "UIHelper.h"
#include "xrUICore/Hint/UIHint.h"
#include "xrUICore/Buttons/UIBtnHint.h"
#include "UITaskWnd.h"
#include "UIFactionWarWnd.h"
#include "UIActorInfo.h"
#include "UIRankingWnd.h"
#include "UILogsWnd.h"
#include "UIScriptWnd.h"
#include "encyclopedia_article.h"

#define PDA_EVENTS_XML "pda_events.xml"

CUIEventsWnd::CUIEventsWnd() :
    CUIWindow("CUIEventsWnd"),
    m_UILeftFrame(NULL),
    m_UIRightWnd(NULL),
    m_UILeftHeader(NULL),
    m_UIAnimation(NULL),
    m_UIMapWnd(NULL),
    m_UITaskInfoWnd(NULL),
    m_ListWnd(NULL),
    m_TaskFilter(NULL)
{
    m_flags.zero();
}

CUIEventsWnd::~CUIEventsWnd()
{
    delete_data(m_UIMapWnd);
    delete_data(m_UITaskInfoWnd);
}

bool CUIEventsWnd::Init()
{
    CUIXml xml;
    if (!xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_EVENTS_XML, false))
        return false;

    CUIXmlInit xml_init;
    xml_init.InitWindow(xml, "main_wnd", 0, this);

    // ИСПРАВЛЕНИЕ: Конструкторы с именами
    m_UILeftFrame = xr_new<CUIFrameWindow>("LeftFrame");
    m_UILeftFrame->SetAutoDelete(true);
    AttachChild(m_UILeftFrame);
    xml_init.InitFrameWindow(xml, "main_wnd:left_frame", 0, m_UILeftFrame);

    m_UILeftHeader = xr_new<CUIFrameLineWnd>("LeftHeader");
    m_UILeftHeader->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_UILeftHeader);
    xml_init.InitFrameLine(xml, "main_wnd:left_frame:left_frame_header", 0, m_UILeftHeader);

    m_UIAnimation = xr_new<CUIAnimatedStatic>();
    m_UIAnimation->SetAutoDelete(true);
    xml_init.InitAnimatedStatic(xml, "main_wnd:left_frame:left_frame_header:anim_static", 0, m_UIAnimation);
    m_UILeftHeader->AttachChild(m_UIAnimation);

    m_UIRightWnd = xr_new<CUIWindow>("RightWindow");
    m_UIRightWnd->SetAutoDelete(true);
    AttachChild(m_UIRightWnd);
    xml_init.InitWindow(xml, "main_wnd:right_frame", 0, m_UIRightWnd);

    m_UIMapWnd = xr_new<CUIMapWnd>(nullptr);
    m_UIMapWnd->SetAutoDelete(false);
    m_UIMapWnd->Init("pda_events.xml", "main_wnd:right_frame:map_wnd", !ShadowOfChernobylMode);

    m_UITaskInfoWnd = xr_new<CUITaskDescrWnd>();
    m_UITaskInfoWnd->SetAutoDelete(false);
    m_UITaskInfoWnd->Init(&xml, "main_wnd:right_frame:task_descr_view");

    // ИСПРАВЛЕНИЕ: Конструктор CUIScrollView с именем
    m_ListWnd = xr_new<CUIScrollView>();
    m_ListWnd->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_ListWnd);
    xml_init.InitScrollView(xml, "main_wnd:left_frame:list", 0, m_ListWnd);

    // ИСПРАВЛЕНИЕ: Конструктор CUITabControl с именем
    m_TaskFilter = xr_new<CUITabControl>();
    m_TaskFilter->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_TaskFilter);

    xml_init.InitTabControl(xml, "main_wnd:left_frame:filter_tab", 0, m_TaskFilter);
    m_TaskFilter->SetWindowName("filter_tab");
    Register(m_TaskFilter);
    AddCallback(m_TaskFilter, TAB_CHANGED, CUIWndCallback::void_function(this, &CUIEventsWnd::OnFilterChanged));

    m_currFilter = eActiveTask;
    //SetDescriptionMode(true);

    m_ui_task_item_xml.Load(CONFIG_PATH, UI_PATH, "job_item.xml");

    return true;
}

void CUIEventsWnd::Update()
{
    if (m_flags.test(flNeedReload)) {
        ReloadList(false);
        m_flags.set(flNeedReload, FALSE);
    }
    inherited::Update();
}

void CUIEventsWnd::Draw()
{
    inherited::Draw();
}

void CUIEventsWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUIEventsWnd::OnFilterChanged(CUIWindow* w, void*)
{
    m_currFilter = (ETaskFilters)m_TaskFilter->GetActiveIndex();
    ReloadList(false);
    if (!GetDescriptionMode())
        SetDescriptionMode(true);
}

void CUIEventsWnd::Reload()
{
    m_flags.set(flNeedReload, TRUE);
}

void CUIEventsWnd::ReloadList(bool bClearOnly)
{
    m_ListWnd->Clear();
    if (bClearOnly) return;

    if (!g_actor) return;
    vGameTasks& tasks = Level().GameTaskManager().GetGameTasks();
    vGameTasks::iterator it = tasks.begin();
    CGameTask* task = NULL;

    for (; it != tasks.end(); ++it)
    {
        task = (*it).game_task;
        R_ASSERT(task);
        R_ASSERT(task->GetObjectivesCount() > 0);

        if (!Filter(task)) continue;
        for (u16 i = 0; i < task->GetObjectivesCount(); ++i)
        {
            if (i == 0)
            {
                CUITaskRootItem* pTaskItem = xr_new<CUITaskRootItem>(this);
                pTaskItem->SetGameTask(task, i);
                m_ListWnd->AddWindow(pTaskItem, true);
            }
            else {
                CUITaskSubItem* pTaskItem = xr_new<CUITaskSubItem>(this);
                pTaskItem->SetGameTask(task, i);
                m_ListWnd->AddWindow(pTaskItem, true);
            }
        }
    }
}

void CUIEventsWnd::Show(bool status)
{
    inherited::Show(status);
    m_UIMapWnd->Show(status);
    m_UITaskInfoWnd->Show(status);

    ReloadList(status == false);

}

bool CUIEventsWnd::Filter(CGameTask* t)
{
    ETaskState task_state = t->Objective(0).GetTaskState();
    switch (m_currFilter)
    {
    case CUIEventsWnd::eActiveTask:
        return task_state & eTaskStateInProgress;
    case CUIEventsWnd::eAccomplishedTask:
        return task_state & eTaskStateCompleted;
    case CUIEventsWnd::eFailedTask:
        return task_state & eTaskStateFail;
    case CUIEventsWnd::eMaxTask:
        return false;
    default:
        return false;
    }
}


void CUIEventsWnd::SetDescriptionMode(bool bMap)
{
    if (bMap) {
        m_UIRightWnd->DetachChild(m_UITaskInfoWnd);
        m_UIRightWnd->AttachChild(m_UIMapWnd);
    }
    else {
        m_UIRightWnd->DetachChild(m_UIMapWnd);
        m_UIRightWnd->AttachChild(m_UITaskInfoWnd);
    }
    m_flags.set(flMapMode, bMap);
}

bool CUIEventsWnd::GetDescriptionMode()
{
    return !!m_flags.test(flMapMode);
}

void CUIEventsWnd::ShowDescription(CGameTask* t, int idx)
{
    if (GetDescriptionMode()) {//map
        SGameTaskObjective& o = t->Objective(idx);
        CMapLocation* ml = o.LinkedMapLocation();

        if (ml && ml->SpotEnabled())
            m_UIMapWnd->SetTargetMap(ml->GetLevelName(), ml->GetPosition(), true);
    }
    else
    {
        SGameTaskObjective& o = t->Objective(0);
        idx = 0;

        m_UITaskInfoWnd->ClearAll();

        if (Actor()->encyclopedia_registry->registry().objects_ptr())
        {
            string512 need_group;
            if (idx == 0)
                strcpy(need_group, *t->m_ID);
            else if (!o.m_article_key.size())
                sprintf_s(need_group, "%s/%d", *t->m_ID, idx);
            else
                sprintf_s(need_group, "%s/%s", *t->m_ID, *o.m_article_key);

            ARTICLE_VECTOR::const_iterator it = Actor()->encyclopedia_registry->registry().objects_ptr()->begin();

            for (; it != Actor()->encyclopedia_registry->registry().objects_ptr()->end(); ++it)
            {
                if (ARTICLE_DATA::eTaskArticle == it->article_type)
                {
                    CEncyclopediaArticle A;
                    A.Load(it->article_id);

                    const shared_str& group = A.data()->group;

                    if (strstr(*group, need_group) == *group)
                    {
                        u32 sz = xr_strlen(need_group);
                        if (group.size() == sz || (*group)[sz] == '/')
                            m_UITaskInfoWnd->AddArticle(&A);
                    }
                    else if (it->article_id == o.m_article_id)
                    {
                        CEncyclopediaArticle A;
                        A.Load(it->article_id);
                        m_UITaskInfoWnd->AddArticle(&A);
                    }
                }
            }
        }
    }

    int sz = m_ListWnd->GetSize();
    for (int i = 0; i < sz; ++i)
    {
        CUITaskSubItem* itm = (CUITaskSubItem*)m_ListWnd->GetItem(i);
        if (itm->OwnerTask() == t && itm->OwnerTask()->ActiveObjective().m_idx == idx)
            itm->MarkSelected(true);
        else
            itm->MarkSelected(false);
    }
}

bool CUIEventsWnd::ItemHasDescription(CUITaskItem* itm)
{
    if (itm->OwnerTask()->ActiveObjective().m_idx == 0)// root
        return itm->OwnerTask()->LinkedMapLocation();
    else
    {
        SGameTaskObjective obj = itm->OwnerTask()->ActiveObjective();
        CMapLocation* ml = obj.LinkedMapLocation();
        bool bHasLocation = (NULL != ml);
        bool bIsMapMode = GetDescriptionMode();
        bool b = (bIsMapMode && bHasLocation && ml->SpotEnabled());
        return b;
    }
}
void CUIEventsWnd::Reset()
{
    inherited::Reset();
    Reload();
}
