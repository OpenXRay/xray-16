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
    friend class CGameTaskManager;

protected:
    CGameTask* m_parent{};
    ETaskState m_task_state;
    ETaskType m_task_type;
    SScriptTaskHelper m_pScriptHelper;

public:
    int m_idx{};
    shared_str m_Title;
    shared_str m_Description;

    // icon
    shared_str m_icon_texture_name;

    // map
    shared_str m_map_hint;
    shared_str m_map_location;
    u16 m_map_object_id{};
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
    SGameTaskObjective();
    SGameTaskObjective(CGameTask* parent, int idx);

    CGameTask* GetParent() const { return m_parent; }

    void SetTaskState(ETaskState state);
    auto GetTaskState() const { return m_task_state; }

    auto GetTaskType() const { return m_task_type; }
    auto LinkedMapLocation() const { return m_linked_map_location; }

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

    auto GetIDX_script() const { return m_idx; }

    auto GetTitle_script() const { return m_Title.c_str(); }
    void SetTitle_script(pcstr title) { m_Title = title; }

    auto GetDescription_script() const { return m_Description.c_str(); }
    void SetDescription_script(pcstr desc) { m_Description = desc; }

    auto GetIconName_script() const { return m_icon_texture_name.c_str(); }
    void SetIconName_script(pcstr tex) { m_icon_texture_name = tex; }

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

class CGameTask : public SGameTaskObjective, public Noncopyable
{
public:
    TASK_ID m_ID;
    u32 m_priority{};
    bool m_read{};

public:
    CGameTask();

    void save(IWriter& stream) override;
    void load(IReader& stream) override;

    void ChangeStateCallback() override;

    // map
    void OnArrived();

    // for scripting access
    auto GetID_script() const { return m_ID.c_str(); }
    void SetID_script(pcstr id) { m_ID = id; }

    auto GetPriority_script() const { return m_priority; }
    void SetPriority_script(int prio) { m_priority = prio; }
};
