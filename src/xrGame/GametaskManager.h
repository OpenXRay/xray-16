#pragma once

#include "GameTaskDefs.h"
#include "Common/object_interfaces.h"

class CGameTaskWrapper;
class CGameTask;
class CMapLocation;

class CGameTaskManager
{
    CGameTaskWrapper* m_gametasks_wrapper;
    vGameTasks* m_gametasks;
    enum
    {
        eChanged = (1 << 0),
        eMultipleTasks = (1 << 1),
    };
    Flags8 m_flags;
    u32 m_actual_frame;

protected:
    void UpdateActiveTask();

public:
    CGameTaskManager();
    ~CGameTaskManager();

    void AllowMultipleTask(bool allow) { m_flags.set(eMultipleTasks, allow); }

    void CleanupTasks() const
    {
        for (auto& taskId : g_active_task_id)
            taskId = nullptr;
    }

    vGameTasks& GetGameTasks();
    CGameTask* HasGameTask(const CMapLocation* ml, bool only_inprocess);
    CGameTask* HasGameTask(const TASK_ID& id, bool only_inprocess);
    CGameTask* GiveGameTaskToActor(const TASK_ID& id, u32 timeToComplete, bool bCheckExisting = true, u32 timer_ttl = 0);
    CGameTask* GiveGameTaskToActor(CGameTask* t, u32 timeToComplete, bool bCheckExisting, u32 timer_ttl);
    void SetTaskState(const TASK_ID& id, ETaskState state, TASK_OBJECTIVE_ID objective_id = ROOT_TASK_OBJECTIVE);
    void SetTaskState(CGameTask* t, ETaskState state, TASK_OBJECTIVE_ID objective_id = ROOT_TASK_OBJECTIVE);

    void UpdateTasks();

    CGameTask* ActiveTask(ETaskType type = eTaskTypeStoryline);
    //void SetActiveTask(const TASK_ID& id, ETaskType type = eTaskTypeStoryline);
    void SetActiveTask(CGameTask* task);
    void SetActiveTask(CGameTask* task, TASK_OBJECTIVE_ID objective_id);
    u32 ActualFrame() const { return m_actual_frame; }
    CGameTask* IterateGet(CGameTask* t, ETaskState state, ETaskType type, bool bForward);
    u32 GetTaskIndex(CGameTask* t, ETaskState state, ETaskType type = eTaskTypeStoryline);
    u32 GetTaskCount(ETaskState state, ETaskType type = eTaskTypeStoryline);
    void MapLocationRelcase(CMapLocation* ml);

    void ResetStorage() { m_gametasks = NULL; };
    void DumpTasks();
};
