#include "pch_script.h"
#include "GametaskManager.h"
#include "alife_registry_wrappers.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "GameTask.h"
#include "Level.h"
#include "map_manager.h"
#include "map_location.h"
#include "Actor.h"
#include "UIGameSP.h"
#include "ui/UIPdaWnd.h"
#include "encyclopedia_article.h"
#include "ui/UIMapWnd.h"
#include "xrCore/buffer_vector.h"

TASK_ID g_active_task_id[eTaskTypeCount];

struct FindTaskByID
{
    TASK_ID id;
    bool b_only_inprocess;
    FindTaskByID(const TASK_ID& s, bool search_only_inprocess) : id(s), b_only_inprocess(search_only_inprocess) {}
    bool operator()(const SGameTaskKey& key)
    {
        if (b_only_inprocess)
            return (id == key.task_id && key.game_task->GetTaskState() == eTaskStateInProgress);
        else
            return (id == key.task_id);
    }
};

bool task_prio_pred(const SGameTaskKey& k1, const SGameTaskKey& k2)
{
    return k1.game_task->m_priority > k2.game_task->m_priority;
}

CGameTaskManager::CGameTaskManager()
{
    m_gametasks_wrapper = xr_new<CGameTaskWrapper>();
    m_gametasks_wrapper->registry().init(0); // actor's id
    m_flags.zero();
    m_flags.set(eChanged, TRUE);
    m_gametasks = NULL;

    for (auto& taskId : g_active_task_id)
    {
        if (taskId.size())
        {
            CGameTask* t = HasGameTask(taskId, true);
            if (t)
            {
                SetActiveTask(t);
            }
        }
    }
}

CGameTaskManager::~CGameTaskManager()
{
    delete_data(m_gametasks_wrapper);
    for (auto& taskId : g_active_task_id)
        taskId = nullptr;
}

vGameTasks& CGameTaskManager::GetGameTasks()
{
    if (!m_gametasks)
    {
        m_gametasks = &m_gametasks_wrapper->registry().objects();
#ifdef DEBUG
        Msg("m_gametasks size=%d", m_gametasks->size());
#endif // #ifdef DEBUG
    }

    return *m_gametasks;
}

CGameTask* CGameTaskManager::HasGameTask(const TASK_ID& id, bool only_inprocess)
{
    FindTaskByID key(id, only_inprocess);
    auto it = std::find_if(GetGameTasks().begin(), GetGameTasks().end(), key);
    if (it != GetGameTasks().end())
        return (*it).game_task;

    return 0;
}

CGameTask* CGameTaskManager::GiveGameTaskToActor(const TASK_ID& id,
    u32 timeToComplete, bool bCheckExisting /*= true*/, u32 timer_ttl /*= 0*/)
{
    if (bCheckExisting && HasGameTask(id, false))
        return nullptr;

    CGameTask* t = xr_new<CGameTask>(id);
    return GiveGameTaskToActor(t, timeToComplete, bCheckExisting, timer_ttl);
}

CGameTask* CGameTaskManager::GiveGameTaskToActor(CGameTask* t, u32 timeToComplete, bool bCheckExisting, u32 timer_ttl)
{
    t->CommitScriptHelperContents();
    if (/* bCheckExisting &&*/ HasGameTask(t->m_ID, true))
    {
        Msg("! task [%s] already inprocess", t->m_ID.c_str());
        VERIFY2(0, make_string("give_task : Task [%s] already inprocess!", t->m_ID.c_str()));
        return NULL;
    }

    m_flags.set(eChanged, TRUE);

    SGameTaskKey& key = GetGameTasks().emplace_back(t->m_ID);
    key.game_task = t;
    t->m_ReceiveTime = Level().GetGameTime();
    t->m_TimeToComplete = t->m_ReceiveTime + timeToComplete * 1000; // ms
    t->m_timer_finish = t->m_ReceiveTime + timer_ttl * 1000; // ms

    std::stable_sort(GetGameTasks().begin(), GetGameTasks().end(), task_prio_pred);

    t->OnArrived();

    if (!m_flags.test(eMultipleTasks))
        SetActiveTask(t);
    else
    {
        const ETaskType taskType = t->GetTaskType();
        CGameTask* activeTask = ActiveTask(taskType);
        if (taskType == eTaskTypeStoryline || taskType == eTaskTypeAdditional)
        {
            if ((activeTask == nullptr) || (activeTask->m_priority > t->m_priority))
            {
                SetActiveTask(t);
            }
        }
    }

    //установить флажок необходимости прочтения тасков в PDA
    if (CurrentGameUI())
        CurrentGameUI()->UpdatePda();

    t->ChangeStateCallback();

    return t;
}

void CGameTaskManager::SetTaskState(CGameTask* task, ETaskState state, TASK_OBJECTIVE_ID objective_id /*= ROOT_TASK_OBJECTIVE*/)
{
    m_flags.set(eChanged, TRUE);

    ETaskType type = eTaskTypeStoryline;
    if (m_flags.test(eMultipleTasks))
        type = task->GetTaskType();

    task->SetTaskState(state, objective_id);

    const bool isRoot      = objective_id == ROOT_TASK_OBJECTIVE;
    const bool isActiveObj = task->ActiveObjectiveIdx() == objective_id;

    if ((isRoot || !task->HasObjectiveInProgress()) && ActiveTask() == task)
    {
        g_active_task_id[type] = "";
    }
    else if (!isRoot && isActiveObj && objective_id != task->GetObjectivesCount(true))
    { // not last objective
        task->SetActiveObjective(objective_id + 1);
    }

    if (CurrentGameUI())
        CurrentGameUI()->UpdatePda();
}

void CGameTaskManager::SetTaskState(const TASK_ID& id, ETaskState state, TASK_OBJECTIVE_ID objective_id /*= ROOT_TASK_OBJECTIVE*/)
{
    const bool objectiveSpecified = objective_id != ROOT_TASK_OBJECTIVE;
    CGameTask* t = HasGameTask(id, objectiveSpecified);
    if (NULL == t)
    {
        Msg("! actor does not has task [%s]%s", *id, objectiveSpecified ? "" : " or it is completed");
        return;
    }
    SetTaskState(t, state, objective_id);
}

void CGameTaskManager::UpdateTasks()
{
    if (Device.Paused())
        return;

    Level().MapManager().DisableAllPointers();

    u32 task_count = GetGameTasks().size();
    if (0 == task_count)
        return;

    {
        typedef buffer_vector<SGameTaskKey> Tasks;
        Tasks tasks(
            xr_alloca(task_count * sizeof(SGameTaskKey)), task_count, GetGameTasks().begin(), GetGameTasks().end());

        for (const SGameTaskKey& taskKey : tasks)
        {
            CGameTask* const t = taskKey.game_task;
            if (t->GetTaskState() != eTaskStateInProgress)
                continue;

            const auto objectives = t->GetObjectivesCount();
            for (TASK_OBJECTIVE_ID i = 0; i < objectives; ++i)
            {
                SGameTaskObjective& obj = t->Objective(i);
                if (obj.GetTaskState() != eTaskStateInProgress)
                    continue;

                ETaskState const state = obj.UpdateState();

                if ((state == eTaskStateFail) || (state == eTaskStateCompleted))
                    SetTaskState(t, state, i);
            }
        }
    }

    for (int i = 0; i < eTaskTypeCount; ++i)
    {
        CGameTask* activeTask = ActiveTask(static_cast<ETaskType>(i));
        if (activeTask)
        {
            CMapLocation* ml = activeTask->LinkedMapLocation();
            if (ml && !ml->PointerEnabled())
                ml->EnablePointer();
        }
    }

    if (m_flags.test(eChanged))
        UpdateActiveTask();
}

void CGameTaskManager::UpdateActiveTask()
{
    std::stable_sort(GetGameTasks().begin(), GetGameTasks().end(), task_prio_pred);

    for (u32 i = eTaskTypeStoryline; i < eTaskTypeCount; ++i)
    {
        CGameTask* activeTask = ActiveTask(static_cast<ETaskType>(i));
        if (!activeTask && (i == eTaskTypeStoryline || m_flags.test(eMultipleTasks)))
        {
            CGameTask* frontTask = IterateGet(nullptr, eTaskStateInProgress, static_cast<ETaskType>(i), true);
            if (frontTask)
                SetActiveTask(frontTask);
        }
    }

    m_flags.set(eChanged, FALSE);
    m_actual_frame = Device.dwFrame;
}

CGameTask* CGameTaskManager::ActiveTask(ETaskType type)
{
    if (type != eTaskTypeStoryline && !m_flags.test(eMultipleTasks))
        return nullptr;

    TASK_ID& t_id = g_active_task_id[type];

    if (!t_id.size())
        return nullptr;

    return HasGameTask(t_id, true);
}

/*
void CGameTaskManager::SetActiveTask(const TASK_ID& id, ETaskType type)
{
    ETaskType t = eTaskTypeStoryline;
    if (m_flags.test(eMultipleTasks))
        t = type;

    g_active_task_id[t] = id;
    m_flags.set(eChanged, TRUE);
    m_read = true;
}*/

void CGameTaskManager::SetActiveTask(CGameTask* task, TASK_OBJECTIVE_ID objective_id)
{
    VERIFY(task);
    if (task)
    {
        ETaskType type = eTaskTypeStoryline;
        if (m_flags.test(eMultipleTasks))
            type = task->GetTaskType();

        g_active_task_id[type] = task->m_ID;
        task->SetActiveObjective(objective_id);

        m_flags.set(eChanged, TRUE);
        task->m_read = true;
    }
}

void CGameTaskManager::SetActiveTask(CGameTask* task)
{
    SetActiveTask(task, task->ActiveObjectiveIdx());
}

CUIMapWnd* GetMapWnd();

void CGameTaskManager::MapLocationRelcase(CMapLocation* ml)
{
    CUIMapWnd* mwnd = GetMapWnd();
    if (mwnd)
        mwnd->MapLocationRelcase(ml);

    CGameTask* gt = HasGameTask(ml, false);
    if (gt)
        gt->RemoveMapLocations(true);
}

CGameTask* CGameTaskManager::HasGameTask(const CMapLocation* ml, bool only_inprocess)
{
    auto it = GetGameTasks().begin();
    auto it_e = GetGameTasks().end();

    for (; it != it_e; ++it)
    {
        CGameTask* gt = (*it).game_task;
        if (gt->LinkedMapLocation() == ml)
        {
            if (only_inprocess && gt->GetTaskState() != eTaskStateInProgress)
                continue;

            return gt;
        }
    }
    return NULL;
}

CGameTask* CGameTaskManager::IterateGet(CGameTask* t, ETaskState state, ETaskType type, bool bForward)
{
    vGameTasks& v = GetGameTasks();
    const ptrdiff_t cnt = v.size();
    for (s32 i = 0; i < cnt; ++i)
    {
        CGameTask* gt = v[i].game_task;
        if (gt == t || NULL == t)
        {
            bool allow;
            if (bForward)
            {
                if (t)
                    ++i;
                allow = i < cnt;
            }
            else
            {
                allow = (i > 0) && (--i >= 0);
            }
            if (allow)
            {
                CGameTask* found = v[i].game_task;
                if (found->GetTaskState() == state && found->GetTaskType() == type)
                    return found;
                else
                    return IterateGet(found, state, type, bForward);
            }
            else
                return NULL;
        }
    }
    return NULL;
}

u32 CGameTaskManager::GetTaskIndex(CGameTask* t, ETaskState state, ETaskType type)
{
    if (!t)
    {
        return 0;
    }

    vGameTasks& v = GetGameTasks();
    u32 cnt = v.size();
    u32 res = 0;
    for (u32 i = 0; i < cnt; ++i)
    {
        CGameTask* gt = v[i].game_task;
        if (gt->GetTaskType() == type && gt->GetTaskState() == state)
        {
            ++res;
            if (gt == t)
            {
                return res;
            }
        }
    }
    return 0;
}

u32 CGameTaskManager::GetTaskCount(ETaskState state, ETaskType type)
{
    vGameTasks& v = GetGameTasks();
    u32 cnt = v.size();
    u32 res = 0;
    for (u32 i = 0; i < cnt; ++i)
    {
        CGameTask* gt = v[i].game_task;
        if (gt->GetTaskType() == type && gt->GetTaskState() == state)
        {
            ++res;
        }
    }
    return res;
}

constexpr pcstr sTaskStates[] = { "TaskStateFail", "TaskStateInProgress", "TaskStateCompleted", "TaskStateDummy" };
constexpr pcstr sTaskTypes[] = { "TaskTypeStoryline", "TaskTypeAdditional", "TaskTypeInsignificant", };
void CGameTaskManager::DumpTasks()
{
    for (auto& it : GetGameTasks())
    {
        const CGameTask* gt = it.game_task;
        Msg("ID=[%s] type=[%s] state=[%s] prio=[%d] ",
            gt->m_ID.c_str(),
            sTaskTypes[gt->GetTaskType()],
            sTaskStates[gt->GetTaskState()],
            gt->m_priority);
    }
}
