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
#include "alife_registry_wrappers.h"
#include "alife_simulator.h"
#include "alife_story_registry.h"
#include "game_object_space.h"
#include "Common/object_broker.h"
#include "xrUICore/XML/UITextureMaster.h"

CUIXml* g_gameTaskXml = nullptr;

ALife::_STORY_ID story_id(cpcstr story_id)
{
    using namespace luabind;
    const int res = object_cast<int>(
        object(
            globals(GEnv.ScriptEngine->lua())
                ["story_ids"]
        )
        [story_id]
    );
    return ALife::_STORY_ID(res);
}

u16 storyId2GameId(ALife::_STORY_ID id)
{
    if (ai().get_alife())
    {
        CSE_ALifeDynamicObject* so = ai().alife().story_objects().object(id, true);
        return (so) ? so->ID : u16(-1);
    }

    const u32 cnt = Level().Objects.o_count();
    for (u32 it = 0; it < cnt; ++it)
    {
        IGameObject* O = Level().Objects.o_get_by_iterator(it);
        CGameObject* GO = smart_cast<CGameObject*>(O);
        if (GO->story_id() == id)
            return GO->ID();
    }
    return u16(-1);
}

SGameTaskObjective::SGameTaskObjective(CGameTask* parent, TASK_OBJECTIVE_ID idx)
    : m_parent(parent),
      m_task_state(eTaskStateDummy),
      m_task_type(eTaskTypeDummy),
      m_idx(idx) {}

CGameTask::CGameTask()
    : SGameTaskObjective(this, ROOT_TASK_OBJECTIVE) {}

void CGameTask::Load(const TASK_ID& id)
{
    m_ID = id;
    static bool successfulLoading = false;
    if (!g_gameTaskXml)
    {
        g_gameTaskXml = new CUIXml();
        successfulLoading = g_gameTaskXml->Load(CONFIG_PATH, "gameplay", "game_tasks.xml", false);
    }
    if (!successfulLoading)
    {
        Msg("Trying to load task [%s], but failed to load gameplay/game_tasks.xml", m_ID.c_str());
        return;
    }

    const XML_NODE task_node = g_gameTaskXml->NavigateToNodeWithAttribute("game_task", "id", *id);

    THROW3(task_node, "game task id = ", id.c_str());
    g_gameTaskXml->SetLocalRoot(task_node);
    m_Title = g_gameTaskXml->Read(g_gameTaskXml->GetLocalRoot(), "title", 0, nullptr);
    m_priority = g_gameTaskXml->ReadAttribInt(g_gameTaskXml->GetLocalRoot(), "prio", -1);

    #ifdef DEBUG
    if (m_priority == u32(-1))
    {
        Msg("Game Task [%s] has no priority", id.c_str());
    }
#endif

    const int tag_num = g_gameTaskXml->GetNodesNum(g_gameTaskXml->GetLocalRoot(), "objective");
    m_Objectives.clear();
    for (int i = 0; i < tag_num; i++)
    {
        const XML_NODE l_root = g_gameTaskXml->NavigateToNode("objective", i);
        g_gameTaskXml->SetLocalRoot(l_root);

        SGameTaskObjective& objective = (i == ROOT_TASK_OBJECTIVE)
                                        ? *this
                                        : m_Objectives.emplace_back(this, i);

        //.
        pcstr tag_text = g_gameTaskXml->Read(l_root, "text", 0, nullptr);
        objective.m_Description = tag_text;

        //.
        tag_text = g_gameTaskXml->Read(l_root, "article", 0, nullptr);
        if (tag_text)
            objective.m_article_id = tag_text;

        //.
        tag_text = g_gameTaskXml->ReadAttrib(l_root, "key", nullptr);
        if (tag_text)
            objective.m_article_key = tag_text;

        //.
        if (i == ROOT_TASK_OBJECTIVE)
        {
            objective.m_icon_texture_name = g_gameTaskXml->Read(g_gameTaskXml->GetLocalRoot(), "icon", 0, nullptr);
            if (objective.m_icon_texture_name.size() &&
                0 != xr_stricmp(*objective.m_icon_texture_name, "ui\\ui_icons_task"))
            {
                objective.m_icon_rect = CUITextureMaster::GetTextureRect(*objective.m_icon_texture_name);
                objective.m_icon_rect.rb.sub(objective.m_icon_rect.rb, objective.m_icon_rect.lt);
                objective.m_icon_texture_name = CUITextureMaster::GetTextureFileName(*objective.m_icon_texture_name);
            }
            else if (objective.m_icon_texture_name.size())
            {
                objective.m_icon_rect.x1 = g_gameTaskXml->ReadAttribFlt(l_root, "icon", 0, "x");
                objective.m_icon_rect.y1 = g_gameTaskXml->ReadAttribFlt(l_root, "icon", 0, "y");
                objective.m_icon_rect.x2 = g_gameTaskXml->ReadAttribFlt(l_root, "icon", 0, "width");
                objective.m_icon_rect.y2 = g_gameTaskXml->ReadAttribFlt(l_root, "icon", 0, "height");
            }
        }
        //.
        objective.m_map_location = g_gameTaskXml->Read(l_root, "map_location_type", 0, nullptr);

        cpcstr object_story_id = g_gameTaskXml->Read(l_root, "object_story_id", 0, nullptr);

        //*
        objective.m_def_location_enabled = !g_gameTaskXml->ReadInt(l_root, "map_location_hidden", 0, 0);

        const bool b1 = (0 == objective.m_map_location.size());
        const bool b2 = (nullptr == object_story_id);
        VERIFY3(b1 == b2, "check [map_location_type] and [object_story_id] fields in objective definition for: ",
            objective.m_Description.c_str());

        //.
        objective.m_map_object_id = u16(-1);

        //.
        objective.m_map_hint = g_gameTaskXml->ReadAttrib(l_root, "map_location_type", 0, "hint", nullptr);

        if (object_story_id)
        {
            const ALife::_STORY_ID _sid = story_id(object_story_id);
            objective.m_map_object_id = storyId2GameId(_sid);
        }

        //------infoportion_complete
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "infoportion_complete");
            objective.m_completeInfos.resize(info_num);

            for (int j = 0; j < info_num; ++j)
            {
                objective.m_completeInfos[j] = g_gameTaskXml->Read(l_root, "infoportion_complete", j, nullptr);
            }
        }

        //------infoportion_fail
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "infoportion_fail");
            objective.m_failInfos.resize(info_num);

            for (int j = 0; j < info_num; ++j)
            {
                objective.m_failInfos[j] = g_gameTaskXml->Read(l_root, "infoportion_fail", j, nullptr);
            }
        }

        //------infoportion_set_complete
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "infoportion_set_complete");
            objective.m_infos_on_complete.resize(info_num);
            for (int j = 0; j < info_num; ++j)
            {
                objective.m_infos_on_complete[j] = g_gameTaskXml->Read(l_root, "infoportion_set_complete", j, nullptr);
            }
        }

        //------infoportion_set_fail
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "infoportion_set_fail");
            objective.m_infos_on_fail.resize(info_num);
            for (int j = 0; j < info_num; ++j)
            {
                objective.m_infos_on_fail[j] = g_gameTaskXml->Read(l_root, "infoportion_set_fail", j, nullptr);
            }
        }

        //------function_complete
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "function_complete");
            objective.m_complete_lua_functions.resize(info_num);
            for (int j = 0; j < info_num; ++j)
            {
                cpcstr str = g_gameTaskXml->Read(l_root, "function_complete", j, nullptr);
                const bool functor_exists = GEnv.ScriptEngine->functor(str, objective.m_complete_lua_functions[j]);
                THROW3(functor_exists, "Cannot find script function described in task objective  ", str);
            }
        }

        //------function_fail
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "function_fail");
            objective.m_fail_lua_functions.resize(info_num);
            for (int j = 0; j < info_num; ++j)
            {
                cpcstr str = g_gameTaskXml->Read(l_root, "function_fail", j, nullptr);
                const bool functor_exists = GEnv.ScriptEngine->functor(str, objective.m_fail_lua_functions[j]);
                THROW3(functor_exists, "Cannot find script function described in task objective  ", str);
            }
        }

        //------function_on_complete
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "function_call_complete");
            objective.m_lua_functions_on_complete.resize(info_num);
            for (int i = 0; i < info_num; ++i)
            {
                cpcstr str = g_gameTaskXml->Read(l_root, "function_call_complete", i, nullptr);
                const bool functor_exists = GEnv.ScriptEngine->functor(str, objective.m_lua_functions_on_complete[i]);
                THROW3(functor_exists, "Cannot find script function described in task objective  ", str);
            }
        }

        //------function_on_fail
        {
            const int info_num = g_gameTaskXml->GetNodesNum(l_root, "function_call_fail");
            objective.m_lua_functions_on_fail.resize(info_num);
            for (int j = 0; j < info_num; ++j)
            {
                cpcstr str = g_gameTaskXml->Read(l_root, "function_call_fail", j, nullptr);
                const bool functor_exists = GEnv.ScriptEngine->functor(str, objective.m_lua_functions_on_fail[j]);
                THROW3(functor_exists, "Cannot find script function described in task objective  ", str);
            }
        }

        g_gameTaskXml->SetLocalRoot(task_node);
    }
    g_gameTaskXml->SetLocalRoot(g_gameTaskXml->GetRoot());
}

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

TASK_OBJECTIVE_ID CGameTask::ActiveObjectiveIdx() const
{
    return m_active_objective;
}

SGameTaskObjective* CGameTask::ActiveObjective()
{
    if (m_active_objective == NO_TASK_OBJECTIVE)
        return nullptr;

    return &Objective(m_active_objective);
}

SGameTaskObjective& CGameTask::Objective(TASK_OBJECTIVE_ID idx)
{
    if (idx == ROOT_TASK_OBJECTIVE)
        return *this;
    return m_Objectives[idx - 1];
}

const SGameTaskObjective& CGameTask::Objective(TASK_OBJECTIVE_ID idx) const
{
    if (idx == ROOT_TASK_OBJECTIVE)
        return *this;
    return m_Objectives[idx - 1];
}

ETaskState CGameTask::ObjectiveState(TASK_OBJECTIVE_ID idx) const
{
    if (idx == NO_TASK_OBJECTIVE)
        return GetTaskState();
    return Objective(idx).GetTaskState();
}

void CGameTask::SetActiveObjective(TASK_OBJECTIVE_ID idx)
{
    VERIFY2(idx != 0, "Objective ID should not be 0.");
    m_active_objective = idx;
}

TASK_OBJECTIVE_ID CGameTask::GetObjectivesCount() const
{
    return m_Objectives.size() + 1; // plus task itself
}

void CGameTask::SetTaskState(ETaskState state, TASK_OBJECTIVE_ID objective_id)
{
    if (objective_id == NO_TASK_OBJECTIVE)
    {
        SetTaskState(state);
        return;
    }
    if (objective_id != ROOT_TASK_OBJECTIVE)
    {
        Objective(objective_id).SetTaskState(state);
        return;
    }
    // Set state for task and all sub-tasks
    if (GetTaskState() == eTaskStateInProgress)
        SetTaskState(state);
    for (SGameTaskObjective& objective : m_Objectives)
    {
        if (objective.GetTaskState() == eTaskStateInProgress)
            objective.SetTaskState(state);
    }
}

bool CGameTask::HasObjectiveInProgress() const
{
    for (const SGameTaskObjective& objective : m_Objectives)
    {
        if (objective.GetTaskState() == eTaskStateInProgress)
            return true;
    }
    return false;
}

void CGameTask::OnArrived()
{
    m_task_state = eTaskStateInProgress;
    m_read = false;

    FillEncyclopedia();
    CreateMapLocation(false);
}

void CGameTask::FillEncyclopedia() const
{
    ARTICLE_VECTOR& article_vector = Actor()->encyclopedia_registry->registry().objects();
    for (const SGameTaskObjective& obj : m_Objectives)
    {
        if (!obj.m_article_id.size())
            continue;

        if (article_vector.end() != std::find_if(article_vector.begin(), article_vector.end(), FindArticleByIDPred(obj.m_article_id)))
            continue;

        CEncyclopediaArticle article;
        article.Load(obj.m_article_id);
        article_vector.emplace_back(obj.m_article_id, Level().GetGameTime(), article.data()->articleType);
    }
}

CMapLocation* CGameTask::LinkedMapLocation()
{
    if (m_active_objective == NO_TASK_OBJECTIVE)
        return m_linked_map_location;

    return Objective(m_active_objective).LinkedMapLocation();
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
    if (!m_Objectives.empty())
    {
        SGameTaskObjective::ChangeStateCallback();
        return;
    }
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

    save_data(m_article_id, stream);
    save_data(m_article_key, stream);

    save_data(m_pScriptHelper, stream);

    save_data(m_icon_rect, stream);
    save_data(m_icon_texture_name, stream);
    save_data(m_def_location_enabled, stream);

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

    load_data(m_article_id, stream);
    load_data(m_article_key, stream);

    load_data(m_pScriptHelper, stream);

    load_data(m_icon_rect, stream);
    load_data(m_icon_texture_name, stream);
    load_data(m_def_location_enabled, stream);

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

void SGameTaskObjective::SetIconName_script(pcstr tex)
{
    m_icon_texture_name = tex;
    m_icon_rect = CUITextureMaster::GetTextureRect(m_icon_texture_name.c_str());
    m_icon_rect.rb.sub(m_icon_rect.rb, m_icon_rect.lt);
}

void SGameTaskObjective::AddCompleteInfo_script(pcstr str) { m_completeInfos.emplace_back(str); }
void SGameTaskObjective::AddCompleteFunc_script(pcstr str) { m_pScriptHelper.m_s_complete_lua_functions.emplace_back(str); }

void SGameTaskObjective::AddOnCompleteInfo_script(pcstr str) { m_infos_on_complete.emplace_back(str); }
void SGameTaskObjective::AddOnCompleteFunc_script(pcstr str) { m_pScriptHelper.m_s_lua_functions_on_complete.emplace_back(str); }

void SGameTaskObjective::AddFailInfo_script(pcstr str) { m_failInfos.emplace_back(str); }
void SGameTaskObjective::AddFailFunc_script(pcstr str) { m_pScriptHelper.m_s_fail_lua_functions.emplace_back(str); }

void SGameTaskObjective::AddOnFailInfo_script(pcstr str) { m_infos_on_fail.emplace_back(str); }
void SGameTaskObjective::AddOnFailFunc_script(pcstr str) { m_pScriptHelper.m_s_lua_functions_on_fail.emplace_back(str); }

void SGameTaskObjective::CommitScriptHelperContents()
{
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_complete_lua_functions, m_complete_lua_functions);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_fail_lua_functions, m_fail_lua_functions);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_complete, m_lua_functions_on_complete);
    m_pScriptHelper.init_functors(m_pScriptHelper.m_s_lua_functions_on_fail, m_lua_functions_on_fail);
}

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
