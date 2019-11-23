#include "pch_script.h"
#include "GameTask.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "encyclopedia_article.h"
#include "map_location.h"
#include "map_spot.h"
#include "map_manager.h"

#include "Level.h"
#include "Actor.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "alife_simulator.h"
#include "alife_story_registry.h"
#include "game_object_space.h"
#include "Common/object_broker.h"
#include "xrUICore/XML/UITextureMaster.h"

SGameTaskObjective::SGameTaskObjective()
    : m_task_state(eTaskStateDummy),
      m_task_type(eTaskTypeDummy) {}

SGameTaskObjective::SGameTaskObjective(CGameTask* parent, int idx)
    : m_parent(parent),
      m_task_state(eTaskStateDummy),
      m_task_type(eTaskTypeDummy),
      m_idx(idx) {}

CGameTask::CGameTask()
    : SGameTaskObjective(this, 0) {}

void SGameTaskObjective::SetTaskState(ETaskState state)
{
    m_task_state = state;
    if ((m_task_state == eTaskStateFail) || (m_task_state == eTaskStateCompleted))
    {
        RemoveMapLocations(false);
        m_FinishTime = Level().GetGameTime();

        if (m_task_state == eTaskStateFail)
        {
            SendInfo(m_infos_on_fail);
            CallAllFuncs(m_lua_functions_on_fail);
        }
        else
        {
            SendInfo(m_infos_on_complete);
            CallAllFuncs(m_lua_functions_on_complete);
        }
    }
    ChangeStateCallback();
}

void CGameTask::OnArrived()
{
    m_task_state = eTaskStateInProgress;
    m_read = false;

    CreateMapLocation(false);
}

void SGameTaskObjective::CreateMapLocation(bool on_load)
{
    if (m_map_object_id == u16(-1) || m_map_location.size() == 0)
    {
        return;
    }

    if (on_load)
    {
        xr_vector<CMapLocation*> res;
        Level().MapManager().GetMapLocations(m_map_location, m_map_object_id, res);
        xr_vector<CMapLocation*>::iterator it = res.begin();
        xr_vector<CMapLocation*>::iterator it_e = res.end();
        for (; it != it_e; ++it)
        {
            CMapLocation* ml = *it;
            if (ml->m_owner_task_id == m_parent->m_ID)
            {
                m_linked_map_location = ml;
                break;
            }
        }
        //.		m_linked_map_location =	Level().MapManager().GetMapLocation(m_map_location, m_map_object_id);
    }
    else
    {
        m_linked_map_location = Level().MapManager().AddMapLocation(m_map_location, m_map_object_id);
        m_linked_map_location->m_owner_task_id = m_parent->m_ID;
    }

    VERIFY(m_linked_map_location);

    if (!on_load)
    {
        if (m_map_hint.size())
        {
            m_linked_map_location->SetHint(m_map_hint);
        }
        m_linked_map_location->DisablePointer();
        m_linked_map_location->SetSerializable(true);
    }

    if (m_linked_map_location->complex_spot())
    {
        m_linked_map_location->complex_spot()->SetTimerFinish(m_timer_finish);
    }
}

void SGameTaskObjective::RemoveMapLocations(bool notify)
{
    if (m_linked_map_location && !notify)
        Level().MapManager().RemoveMapLocation(m_linked_map_location);

    m_map_location = nullptr;
    m_linked_map_location = nullptr;
    m_map_object_id = u16(-1);
}

void SGameTaskObjective::ChangeMapLocation(pcstr new_map_location, u16 new_map_object_id)
{
    RemoveMapLocations(false);

    m_map_location = new_map_location;
    m_map_object_id = new_map_object_id;

    m_task_state = eTaskStateInProgress;
    CreateMapLocation(false);
}

void SGameTaskObjective::ChangeStateCallback()
{
    Actor()->callback(GameObject::eTaskStateChange)(GetParent(), this, GetTaskState());
}

void CGameTask::ChangeStateCallback()
{
    Actor()->callback(GameObject::eTaskStateChange)(this, GetTaskState());
}

ETaskState SGameTaskObjective::UpdateState()
{
    if (m_idx == 0 && m_ReceiveTime != m_TimeToComplete)
    {
        if (Level().GetGameTime() > m_TimeToComplete)
        {
            return eTaskStateFail;
        }
    }

    // check fail infos
    if (CheckInfo(m_failInfos))
        return eTaskStateFail;

    // check fail functor
    if (CheckFunctions(m_fail_lua_functions))
        return eTaskStateFail;

    // check complete infos
    if (CheckInfo(m_completeInfos))
        return eTaskStateCompleted;

    // check complete functor
    if (CheckFunctions(m_complete_lua_functions))
        return eTaskStateCompleted;

    return GetTaskState();
}

bool SGameTaskObjective::CheckInfo(const xr_vector<shared_str>& v) const
{
    bool res = false;
    xr_vector<shared_str>::const_iterator it = v.begin();
    for (; it != v.end(); ++it)
    {
        res = Actor()->HasInfo(*it);
        if (!res)
            break;
    }
    return res;
}

bool SGameTaskObjective::CheckFunctions(const task_state_functors& v) const
{
    bool res = false;
    task_state_functors::const_iterator it = v.begin();
    for (; it != v.end(); ++it)
    {
        if ((*it).is_valid())
            res = (*it)(m_parent->m_ID.c_str());
        if (!res)
            break;
    }
    return res;
}

void SGameTaskObjective::CallAllFuncs(const task_state_functors& v)
{
    task_state_functors::const_iterator it = v.begin();
    for (; it != v.end(); ++it)
    {
        if ((*it).is_valid())
            (*it)(m_parent->m_ID.c_str());
    }
}

void SGameTaskObjective::SendInfo(const xr_vector<shared_str>& v)
{
    xr_vector<shared_str>::const_iterator it = v.begin();
    for (; it != v.end(); ++it)
        Actor()->TransferInfo((*it), true);
}

void SGameTaskObjective::save(IWriter& stream)
{
    save_data(m_task_state, stream);
    save_data(m_task_type, stream);

    save_data(m_ReceiveTime, stream);
    save_data(m_FinishTime, stream);
    save_data(m_TimeToComplete, stream);
    save_data(m_timer_finish, stream);

    save_data(m_idx, stream);
    save_data(m_Title, stream);
    save_data(m_Description, stream);

    save_data(m_pScriptHelper, stream);

    save_data(m_icon_texture_name, stream);
    save_data(m_map_hint, stream);
    save_data(m_map_location, stream);
    save_data(m_map_object_id, stream);
}

void SGameTaskObjective::load(IReader& stream)
{
    load_data(m_task_state, stream);
    load_data(m_task_type, stream);

    load_data(m_ReceiveTime, stream);
    load_data(m_FinishTime, stream);
    load_data(m_TimeToComplete, stream);
    load_data(m_timer_finish, stream);

    load_data(m_idx, stream);
    load_data(m_Title, stream);
    load_data(m_Description, stream);

    load_data(m_pScriptHelper, stream);

    load_data(m_icon_texture_name, stream);
    load_data(m_map_hint, stream);
    load_data(m_map_location, stream);
    load_data(m_map_object_id, stream);
}

void CGameTask::save(IWriter& stream)
{
    save_data(m_ID, stream);
    SGameTaskObjective::save(stream);
    save_data(m_priority, stream);
}

void CGameTask::load(IReader& stream)
{
    load_data(m_ID, stream);
    SGameTaskObjective::load(stream);
    load_data(m_priority, stream);

    CommitScriptHelperContents();
    CreateMapLocation(true);
}

void SGameTaskObjective::CommitScriptHelperContents()
{
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_complete_lua_functions, m_complete_lua_functions);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_fail_lua_functions, m_fail_lua_functions);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_complete, m_lua_functions_on_complete);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_fail, m_lua_functions_on_fail);
}

void SGameTaskObjective::AddCompleteInfo_script(pcstr str) { m_completeInfos.emplace_back(str); }
void SGameTaskObjective::AddCompleteFunc_script(pcstr str) { m_pScriptHelper.m_s_complete_lua_functions.emplace_back(str); }

void SGameTaskObjective::AddOnCompleteInfo_script(pcstr str) { m_infos_on_complete.emplace_back(str); }
void SGameTaskObjective::AddOnCompleteFunc_script(pcstr str) { m_pScriptHelper.m_s_lua_functions_on_complete.emplace_back(str); }

void SGameTaskObjective::AddFailInfo_script(pcstr str) { m_failInfos.emplace_back(str); }
void SGameTaskObjective::AddFailFunc_script(pcstr str) { m_pScriptHelper.m_s_fail_lua_functions.emplace_back(str); }

void SGameTaskObjective::AddOnFailInfo_script(pcstr str) { m_infos_on_fail.emplace_back(str); }
void SGameTaskObjective::AddOnFailFunc_script(pcstr str) { m_pScriptHelper.m_s_lua_functions_on_fail.emplace_back(str); }

void SScriptTaskHelper::init_functors(xr_vector<shared_str>& v_src, task_state_functors& v_dest)
{
    auto it = v_src.begin();
    auto it_e = v_src.end();
    v_dest.resize(v_src.size());

    for (u32 idx = 0; it != it_e; ++it, ++idx)
    {
        const bool functor_exists = GEnv.ScriptEngine->functor(*(*it), v_dest[idx]);
        if (!functor_exists)
            Log("Cannot find script function described in task objective  ", *(*it));
    }
}

void SScriptTaskHelper::load(IReader& stream)
{
    load_data(m_s_complete_lua_functions, stream);
    load_data(m_s_fail_lua_functions, stream);
    load_data(m_s_lua_functions_on_complete, stream);
    load_data(m_s_lua_functions_on_fail, stream);
}

void SScriptTaskHelper::save(IWriter& stream)
{
    save_data(m_s_complete_lua_functions, stream);
    save_data(m_s_fail_lua_functions, stream);
    save_data(m_s_lua_functions_on_complete, stream);
    save_data(m_s_lua_functions_on_fail, stream);
}

void SGameTaskKey::save(IWriter& stream)
{
    R_ASSERT(task_id == game_task->m_ID);
    save_data(game_task, stream);
}

void SGameTaskKey::load(IReader& stream)
{
    load_data(game_task, stream);
    task_id = game_task->m_ID;
}

void SGameTaskKey::destroy() { delete_data(game_task); }
