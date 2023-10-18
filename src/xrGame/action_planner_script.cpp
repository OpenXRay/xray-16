////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_script.cpp
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_action_planner_wrapper.h"
#include "script_game_object.h"
#include "action_base.h"
#include "xrScriptEngine/ScriptExporter.hpp"

void set_goal_world_state(CScriptActionPlanner* action_planner, CScriptActionPlanner::CState* world_state)
{
    action_planner->set_target_state(*world_state);
}

bool get_actual(const CScriptActionPlanner* action_planner) { return (action_planner->actual()); }

CScriptActionPlanner* cast_planner(CScriptActionBase* action) { return (smart_cast<CScriptActionPlanner*>(action)); }

IC static void CScriptActionPlanner_Export(lua_State* luaState)
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CScriptActionPlanner, no_bases, default_holder, CScriptActionPlannerWrapper>("action_planner")
            .def_readonly("object", &CScriptActionPlanner::m_object)
            .def_readonly("storage", &CScriptActionPlanner::m_storage)
            .def(constructor<>())
            .def("actual", &get_actual)
            .def("setup", &CScriptActionPlanner::setup, &CScriptActionPlannerWrapper::setup_static)
            .def("update", &CScriptActionPlanner::update, &CScriptActionPlannerWrapper::update_static)
            .def("add_action", &CScriptActionPlanner::add_operator, adopt<3>())
            .def("remove_action", (void (CScriptActionPlanner::*)(const CScriptActionPlanner::_edge_type&))(
                                      &CScriptActionPlanner::remove_operator))
            .def("action", &CScriptActionPlanner::action)
            .def("add_evaluator", &CScriptActionPlanner::add_evaluator, adopt<3>())
            .def("remove_evaluator",
                (void (CScriptActionPlanner::*)(const CScriptActionPlanner::_condition_type&))(
                    &CScriptActionPlanner::remove_evaluator))
            .def("evaluator", &CScriptActionPlanner::evaluator)
            .def("current_action_id", &CScriptActionPlanner::current_action_id)
            .def("current_action", &CScriptActionPlanner::current_action)
            .def("initialized", &CScriptActionPlanner::initialized)
            .def("set_goal_world_state", &set_goal_world_state)
            .def("clear", &CScriptActionPlanner::clear)
#ifdef LOG_ACTION
            .def("show", &CScriptActionPlanner::show)
            .def("show_current_world_state", &CScriptActionPlanner::show_current_world_state)
            .def("show_target_world_state", &CScriptActionPlanner::show_target_world_state)
#endif // LOG_ACTION

            ,

        def("cast_planner", &cast_planner)
    ];
}

SCRIPT_EXPORT_FUNC(CScriptActionPlanner, (), CScriptActionPlanner_Export);
