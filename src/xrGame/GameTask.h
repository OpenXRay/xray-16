#pragma once
#include "encyclopedia_article_defs.h"
#include "GameTaskDefs.h"
struct lua_State;
// XXX: include "xrScriptEngine/script_space_forward.hpp" into Functor.hpp, define functor
#include "xrScriptEngine/Functor.hpp"

class CGameTaskManager;
class CMapLocation;
class CGameTask;

typedef xr_vector<luabind::functor<bool>> task_state_functors;

class SScriptTaskHelper : public ISerializable
{
public:
    xr_vector<shared_str> m_s_complete_lua_functions;
    xr_vector<shared_str> m_s_fail_lua_functions;

    xr_vector<shared_str> m_s_lua_functions_on_complete;
    xr_vector<shared_str> m_s_lua_functions_on_fail;

public:
    bool not_empty()
    {
        return m_s_complete_lua_functions.size() || m_s_fail_lua_functions.size() ||
            m_s_lua_functions_on_complete.size() || m_s_lua_functions_on_fail.size();
    }

    virtual void save(IWriter& stream);
    virtual void load(IReader& stream);

    void init_functors(xr_vector<shared_str>& v_src, task_state_functors& v_dest);
};

class SGameTaskObjective : public ISerializable
{
    friend struct SGameTaskKey;
    friend class CGameTask;
    friend class CGameTaskManager;

protected:
    CGameTask* m_parent{};
    ETaskState m_task_state;
    ETaskType m_task_type;
    SScriptTaskHelper m_pScriptHelper;

public:
    TASK_OBJECTIVE_ID m_idx;
    shared_str m_Title;
    shared_str m_Description;

    // encyclopedia
    shared_str m_article_id;
    shared_str m_article_key;

    // icon
    Frect m_icon_rect;
    shared_str m_icon_texture_name;

    // map
    shared_str m_map_hint;
    shared_str m_map_location;
    u16 m_map_object_id{};
    bool m_def_location_enabled{};
    CMapLocation* m_linked_map_location{};

    // timing
    ALife::_TIME_ID m_ReceiveTime{};
    ALife::_TIME_ID m_FinishTime{};
    ALife::_TIME_ID m_TimeToComplete{};
    ALife::_TIME_ID m_timer_finish{};

private:
    // infos
    xr_vector<shared_str> m_completeInfos;
    xr_vector<shared_str> m_failInfos;
    xr_vector<shared_str> m_infos_on_complete;
    xr_vector<shared_str> m_infos_on_fail;

    // functions
    task_state_functors m_fail_lua_functions;
    task_state_functors m_complete_lua_functions;

    task_state_functors m_lua_functions_on_complete;
    task_state_functors m_lua_functions_on_fail;

public:
    SGameTaskObjective(CGameTask* parent, TASK_OBJECTIVE_ID idx);

    CGameTask* GetParent() const { return m_parent; }

    void SetTaskState(ETaskState state);
    auto GetTaskState() const { return m_task_state; }

    auto GetTaskType() const { return m_task_type; }
    virtual CMapLocation* LinkedMapLocation() { return m_linked_map_location; }

    ETaskState UpdateState();

    void save(IWriter& stream) override;
    void load(IReader& stream) override;

private:
    void SendInfo(const xr_vector<shared_str>&);
    bool CheckInfo(const xr_vector<shared_str>&) const;
    void CallAllFuncs(const task_state_functors& v);
    bool CheckFunctions(const task_state_functors& v) const;

protected:
    virtual void ChangeStateCallback();
    void CreateMapLocation(bool on_load);

public:
    void RemoveMapLocations(bool notify);
    void ChangeMapLocation(pcstr new_map_location, u16 new_map_object_id);

    // for scripting access
    auto GetType_script() const { return m_task_type; }
    void SetType_script(int t)  { m_task_type = (ETaskType)t; }

    auto GetID() const { return m_idx; }

    auto GetTitle_script() const { return m_Title.c_str(); }
    void SetTitle_script(pcstr title) { m_Title = title; }

    auto GetDescription_script() const { return m_Description.c_str(); }
    void SetDescription_script(pcstr desc) { m_Description = desc; }

    // encyclopedia
    void SetArticleID_script(cpcstr id) { m_article_id = id; }
    void SetArticleKey_script(cpcstr key) { m_article_key = key; }

    // icon
    auto GetIconName_script() const { return m_icon_texture_name.c_str(); }
    void SetIconName_script(pcstr tex);

    // map
    void SetMapHint_script(pcstr hint) { m_map_hint = hint; }
    void SetMapLocation_script(pcstr mls) { m_map_location = mls; }
    void SetMapObjectID_script(int id) { m_map_object_id = (u16)id; }

    // callbacks and infos
    void AddCompleteInfo_script(pcstr str);
    void AddCompleteFunc_script(pcstr str);

    void AddOnCompleteInfo_script(pcstr str);
    void AddOnCompleteFunc_script(pcstr str);

    void AddFailInfo_script(pcstr str);
    void AddFailFunc_script(pcstr str);

    void AddOnFailInfo_script(pcstr str);
    void AddOnFailFunc_script(pcstr str);

    void CommitScriptHelperContents();
};

using OBJECTIVES_VECTOR = xr_vector<SGameTaskObjective>;

class CGameTask final : public SGameTaskObjective, public Noncopyable
{
public:
    TASK_ID m_ID;
    u32 m_priority{};
    bool m_read{};

private:
    OBJECTIVES_VECTOR m_Objectives;
    TASK_OBJECTIVE_ID m_active_objective{ NO_TASK_OBJECTIVE };

public:
    CGameTask();

    void Load(const TASK_ID& id);

    void save(IWriter& stream) override;
    void load(IReader& stream) override;

    void ChangeStateCallback() override;

    TASK_OBJECTIVE_ID ActiveObjectiveIdx() const;
    SGameTaskObjective* ActiveObjective();
    SGameTaskObjective& Objective(TASK_OBJECTIVE_ID idx);
    const SGameTaskObjective& Objective(TASK_OBJECTIVE_ID idx) const;
    ETaskState ObjectiveState(TASK_OBJECTIVE_ID idx) const;
    void SetActiveObjective(TASK_OBJECTIVE_ID idx);
    TASK_OBJECTIVE_ID GetObjectivesCount() const;

    using SGameTaskObjective::SetTaskState;
    void SetTaskState(ETaskState state, TASK_OBJECTIVE_ID objective_id);
    bool HasObjectiveInProgress() const;

    // map
    void OnArrived();
    CMapLocation* LinkedMapLocation() override;

    void FillEncyclopedia() const;

    // for scripting access
    void Load_script(pcstr id) { Load(id); }

    auto GetID_script() const { return m_ID.c_str(); }
    void SetID_script(pcstr id) { m_ID = id; }

    auto GetPriority_script() const { return m_priority; }
    void SetPriority_script(int prio) { m_priority = prio; }

    void AddObjective_script(SGameTaskObjective* O);
    SGameTaskObjective* GetObjective_script(TASK_OBJECTIVE_ID objective_id);
};
