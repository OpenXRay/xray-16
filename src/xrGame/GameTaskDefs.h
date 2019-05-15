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

constexpr pcstr g_active_task_no_task___internal = "__xr_no_task_-_nullptr__";

extern shared_str g_active_task_id[eTaskTypeCount];
class CGameTask;

struct SGameTaskKey : public ISerializable, public IPureDestroyableObject
{
    shared_str task_id;
    CGameTask* game_task;
    SGameTaskKey(const shared_str& t_id) : task_id(t_id), game_task(NULL){};
    SGameTaskKey() : task_id(NULL), game_task(NULL){};

    virtual void save(IWriter& stream);
    virtual void load(IReader& stream);
    virtual void destroy();
};

using vGameTasks = xr_vector<SGameTaskKey>;

struct CGameTaskRegistry : public CALifeAbstractRegistry<u16, vGameTasks>
{
    virtual void save(IWriter& stream)
    {
        CALifeAbstractRegistry<u16, vGameTasks>::save(stream);
        for (auto& taskId : g_active_task_id)
        {
            // valid taskId should contain task name
            // or at least g_active_task_no_task___internal
            if (!taskId.size())
                taskId = g_active_task_no_task___internal;

            save_data(taskId, stream);
        }
    };
    virtual void load(IReader& stream)
    {
        CALifeAbstractRegistry<u16, vGameTasks>::load(stream);

        auto prevPos = stream.tell();

        for (auto& taskId : g_active_task_id)
        {
            // valid taskId should contain task name
            // or at least g_active_task_no_task___internal

            load_data(taskId, stream);

            // if it doesn't fit terms above, then it's not valid
            // probably save file is old. We can try to
            // preserve compatibility with just stream rollback.
            // additionally, if we loaded eTaskTypeAdditional
            // eTaskTypeInsignificant can be empty and it's normal
            if (taskId.size() || taskId == g_active_task_id[eTaskTypeInsignificant])
            {
                prevPos = stream.tell(); // it's valid, remember new pos
            }
            else
            {
                taskId = g_active_task_no_task___internal;
                stream.seek(prevPos); // rollback
                break; // there's no point to continue
            }
        }
    }
};
