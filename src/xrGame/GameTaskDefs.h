#pragma once
#include "alife_abstract_registry.h"

enum ETaskState : u32
{
    eTaskStateFail = 0,
    eTaskStateInProgress,
    eTaskStateCompleted,
    eTaskStateDummy = u16(-1)
};

// all task has `storyline`-type now (10.10.2008)(sea)
// reverted sea changes (13.05.2019)(xottab_duty)
enum ETaskType
{
    eTaskTypeStoryline = 0,
    eTaskTypeAdditional,
    eTaskTypeInsignificant,
    eTaskTypeCount,
    eTaskTypeDummy = u16(-1)
};

using TASK_ID = shared_str;
using TASK_OBJECTIVE_ID = u16;

using TASK_ID_VECTOR = xr_vector<TASK_ID>;

constexpr auto NO_TASK_OBJECTIVE = static_cast<TASK_OBJECTIVE_ID>(-1);
constexpr auto ROOT_TASK_OBJECTIVE = static_cast<TASK_OBJECTIVE_ID>(0); // task itself

extern TASK_ID g_active_task_id[eTaskTypeCount];

class CGameTask;
struct SGameTaskKey : public ISerializable, public IPureDestroyableObject
{
    TASK_ID task_id;
    CGameTask* game_task;
    SGameTaskKey(const TASK_ID& t_id) : task_id(t_id), game_task(NULL){};
    SGameTaskKey() : task_id(NULL), game_task(NULL){};

    virtual void save(IWriter& stream);
    virtual void load(IReader& stream);
    virtual void destroy();
};

using vGameTasks = xr_vector<SGameTaskKey>;

struct CGameTaskRegistry : public CALifeAbstractRegistry<u16, vGameTasks>
{
    void save(IWriter& stream) override
    {
        CALifeAbstractRegistry<u16, vGameTasks>::save(stream);
        for (auto& taskId : g_active_task_id)
        {
            save_data(taskId, stream);
        }
    }

    void load(IReader& stream) override
    {
        CALifeAbstractRegistry<u16, vGameTasks>::load(stream);
        for (auto& taskId : g_active_task_id)
        {
            load_data(taskId, stream);
        }
    }
};
