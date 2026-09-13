#include "StdAfx.h"
#include "UIEventsWnd.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "UIMapWnd.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "UITaskDescrWnd.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "../HUDManager.h"
#include "../Level.h"
#include "../Actor.h"
#include "../GametaskManager.h"
#include "../GameTask.h"
#include "../map_manager.h"
#include "../map_location.h"
#include "xrEngine/StringTable/StringTable.h"
#include "UITaskItem.h"
#include "../alife_registry_wrappers.h"
#include "../encyclopedia_article.h"

CUIEventsWnd::CUIEventsWnd() : CUIWindow("CUIEventsWnd") { m_flags.zero(); }

CUIEventsWnd::~CUIEventsWnd()
{
    delete_data(m_UIMapWnd);
    delete_data(m_UITaskInfoWnd);
}

void CUIEventsWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "pda_events.xml");
    R_ASSERT3(xml_result, "xml file not found", "pda_events.xml");

    CUIXmlInit xml_init;
    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    m_UILeftFrame = xr_new<CUIFrameWindow>();
    m_UILeftFrame->SetAutoDelete(true);
    AttachChild(m_UILeftFrame);
    xml_init.InitFrameWindow(uiXml, "main_wnd:left_frame", 0, m_UILeftFrame);

    m_UILeftHeader = xr_new<CUIFrameLineWnd>();
    m_UILeftHeader->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_UILeftHeader);
    xml_init.InitFrameLine(uiXml, "main_wnd:left_frame:left_frame_header", 0, m_UILeftHeader);

    if (uiXml.NavigateToNode("main_wnd:left_frame:left_frame_header:anim_static"))
    {
        m_UIAnimation = xr_new<CUIAnimatedStatic>();
        m_UIAnimation->SetAutoDelete(true);
        xml_init.InitAnimatedStatic(uiXml, "main_wnd:left_frame:left_frame_header:anim_static", 0, m_UIAnimation);
        m_UILeftHeader->AttachChild(m_UIAnimation);
    }

    m_UIRightWnd = xr_new<CUIWindow>();
    m_UIRightWnd->SetAutoDelete(true);
    AttachChild(m_UIRightWnd);
    xml_init.InitWindow(uiXml, "main_wnd:right_frame", 0, m_UIRightWnd);

    m_UIMapWnd = xr_new<CUIMapWnd>(nullptr);
    m_UIMapWnd->SetAutoDelete(false);
    m_UIMapWnd->Init("pda_events.xml", "main_wnd:right_frame:map_wnd", false);

    m_UITaskInfoWnd = xr_new<CUITaskDescrWnd>();
    m_UITaskInfoWnd->SetAutoDelete(false);
    m_UITaskInfoWnd->Init(&uiXml, "main_wnd:right_frame:task_descr_view");

    m_ListWnd = xr_new<CUIScrollView>();
    m_ListWnd->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_ListWnd);
    xml_init.InitScrollView(uiXml, "main_wnd:left_frame:list", 0, m_ListWnd);

    m_TaskFilter = xr_new<CUITabControl>();
    m_TaskFilter->SetAutoDelete(true);
    m_UILeftFrame->AttachChild(m_TaskFilter);
    xml_init.InitTabControl(uiXml, "main_wnd:left_frame:filter_tab", 0, m_TaskFilter);
    m_TaskFilter->SetWindowName("filter_tab");
    Register(m_TaskFilter);
    AddCallbackStr("filter_tab", TAB_CHANGED, fastdelegate::MakeDelegate(this, &CUIEventsWnd::OnFilterChanged));

    m_currFilter = eActiveTask;
    SetDescriptionMode(true);

    m_ui_task_item_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "job_item.xml");
}

void CUIEventsWnd::Update()
{
    if (m_flags.test(flNeedReload))
    {
        ReloadList(false);
        m_flags.set(flNeedReload, FALSE);
    }
    inherited::Update();
}

void CUIEventsWnd::Draw() { inherited::Draw(); }

void CUIEventsWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }

void CUIEventsWnd::OnFilterChanged(CUIWindow* w, void*)
{
    m_currFilter = (ETaskFilters)m_TaskFilter->GetActiveIndex();
    ReloadList(false);
    if (!GetDescriptionMode())
        SetDescriptionMode(true);
}

void CUIEventsWnd::Reload() { m_flags.set(flNeedReload, TRUE); }

void CUIEventsWnd::ReloadList(bool bClearOnly)
{
    m_ListWnd->Clear();
    if (bClearOnly)
        return;
    if (!g_actor)
        return;

    vGameTasks& tasks = Level().GameTaskManager().GetGameTasks();
    xr_vector<CGameTask*> game_tasks;
    for (const auto& it : tasks)
    {
        CGameTask* task = it.game_task;
        R_ASSERT(task);
        R_ASSERT(task->GetObjectivesCount() > 0);
        if (Filter(task))
            game_tasks.push_back(task);
    }

    if (m_currFilter == eActiveTask)
        std::sort(game_tasks.begin(), game_tasks.end(), [](const auto& a, const auto& b) {
            if (a->m_priority == b->m_priority)
                return a->m_ReceiveTime > b->m_ReceiveTime;
            return a->m_priority < b->m_priority;
        });

    for (const auto& task : game_tasks)
    {
        CUISocTaskItem* pTaskItem = NULL;
        /*
                if(task->m_Objectives[0].TaskState()==eTaskUserDefined)
                {
                    VERIFY				(task->m_Objectives.size()==1);
                    pTaskItem			= xr_new<CUIUserTaskItem>(this);
                    pTaskItem->SetGameTask			(task, 0);
                    m_ListWnd->AddWindow			(pTaskItem,true);
                }else
        */
        const u32 visible_objectives = task->GetObjectivesCount();

        for (u32 i = 0; i < visible_objectives; ++i)
        {
            if (i == 0)
            {
                pTaskItem = xr_new<CUITaskRootItem>(this);
            }
            else
            {
                pTaskItem = xr_new<CUITaskSubItem>(this);
            }
            pTaskItem->SetGameTask(task, (u16)i);
            m_ListWnd->AddWindow(pTaskItem, true);
        }
    }
}

void CUIEventsWnd::Show(bool status)
{
    inherited::Show(status);

    if (GetDescriptionMode())
        m_UIMapWnd->Show(status);
    else
        m_UITaskInfoWnd->Show(status);

    ReloadList(status == false);
}

bool CUIEventsWnd::Filter(CGameTask* t)
{
    const ETaskState task_state = t->ObjectiveState(ROOT_TASK_OBJECTIVE);
    //	bool bprimary_only			= m_primary_or_all_filter_btn->GetCheck();

    return (false /*m_currFilter==eOwnTask && task_state==eTaskUserDefined*/) ||
        ((true /*!bprimary_only || (bprimary_only && t->m_is_task_general)*/) &&
         ((m_currFilter == eAccomplishedTask && task_state == eTaskStateCompleted) || (m_currFilter == eFailedTask && task_state == eTaskStateFail) ||
          (m_currFilter == eActiveTask && task_state == eTaskStateInProgress)));
}

void CUIEventsWnd::SetDescriptionMode(bool bMap)
{
    if (bMap)
    {
        m_descriptionTask = nullptr;
        if (m_UIRightWnd->IsChild(m_UITaskInfoWnd))
            m_UIRightWnd->DetachChild(m_UITaskInfoWnd);
        if (!m_UIRightWnd->IsChild(m_UIMapWnd))
            m_UIRightWnd->AttachChild(m_UIMapWnd);
    }
    else
    {
        if (m_UIRightWnd->IsChild(m_UIMapWnd))
            m_UIRightWnd->DetachChild(m_UIMapWnd);
        if (!m_UIRightWnd->IsChild(m_UITaskInfoWnd))
            m_UIRightWnd->AttachChild(m_UITaskInfoWnd);
    }
    m_flags.set(flMapMode, bMap);
}

bool CUIEventsWnd::GetDescriptionMode() { return !!m_flags.test(flMapMode); }

bool CUIEventsWnd::IsTaskDescriptionShown(const CGameTask* task) const { return m_descriptionTask == task; }

void CUIEventsWnd::ShowDescription(CGameTask* t, int idx)
{
    if (GetDescriptionMode())
    { // map
        SGameTaskObjective& o = t->Objective(idx);
        CMapLocation* ml = o.LinkedMapLocation();

        if (ml && ml->SpotEnabled())
        {
            ml->CalcPosition();
            m_UIMapWnd->SetTargetMap(ml->GetLevelName(), ml->GetPosition(), true);
        }

        int sz = m_ListWnd->GetSize();

        for (int i = 0; i < sz; ++i)
        {
            CUISocTaskItem* itm = (CUISocTaskItem*)m_ListWnd->GetItem(i);

            if ((itm->GameTask() == t) && (itm->ObjectiveIdx() == idx))
                itm->MarkSelected(true);
            else
                itm->MarkSelected(false);
        }
    }
    else
    { // articles
        m_descriptionTask = t;
        const SGameTaskObjective& rootObjective = t->Objective(ROOT_TASK_OBJECTIVE);

        m_UITaskInfoWnd->ClearAll();

        // The task owns its explicit description article. Load it directly so
        // old saves also work when their article registry is incomplete.
        const shared_str explicitArticle = rootObjective.m_article_id;
        if (explicitArticle.size())
            m_UITaskInfoWnd->AddArticle(explicitArticle.c_str());

        if (Actor()->encyclopedia_registry->registry().objects_ptr())
        {
            ARTICLE_VECTOR::const_iterator it = Actor()->encyclopedia_registry->registry().objects_ptr()->begin();

            for (; it != Actor()->encyclopedia_registry->registry().objects_ptr()->end(); ++it)
            {
                if (ARTICLE_DATA::eTaskArticle == it->article_type)
                {
                    if (explicitArticle.size() && it->article_id == explicitArticle)
                        continue;

                    CEncyclopediaArticle A;
                    A.Load(it->article_id);

                    if (t->m_ID == A.data()->group)
                    {
                        m_UITaskInfoWnd->AddArticle(&A);
                    }
                }
            }
        }
    }
}

bool CUIEventsWnd::ItemHasDescription(CUISocTaskItem* itm)
{
    if (itm->ObjectiveIdx() == 0) // root
    {
        CGameTask* task = itm->GameTask();
        const TASK_OBJECTIVE_ID count = task->GetObjectivesCount();
        for (TASK_OBJECTIVE_ID index = 0; index < count; ++index)
        {
            if (task->Objective(index).LinkedMapLocation())
                return true;
        }
        return false;
    }
    else
    {
        SGameTaskObjective* obj = itm->Objective();
        CMapLocation* ml = obj->LinkedMapLocation();
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
