////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_action_script.cpp
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner action script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_action_planner_action_wrapper.h"
#include "script_game_object.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

IC static void CScriptActionPlannerAction_Export(lua_State* luaState)
{
    module(luaState)[class_<CScriptActionPlannerAction, bases<CScriptActionPlanner, CScriptActionBase>, default_holder,
        CScriptActionPlannerActionWrapper>("planner_action")
                         .def(constructor<>())
                         .def(constructor<CScriptGameObject*>())
                         .def(constructor<CScriptGameObject*, LPCSTR>())
                         .def("setup", &CScriptActionPlannerAction::setup,
                             &CScriptActionPlannerActionWrapper::setup_static)
                         .def("initialize", &CScriptActionPlannerAction::initialize,
                             &CScriptActionPlannerActionWrapper::initialize_static)
                         .def("execute", &CScriptActionPlannerAction::execute,
                             &CScriptActionPlannerActionWrapper::execute_static)
                         .def("finalize", &CScriptActionPlannerAction::finalize,
                             &CScriptActionPlannerActionWrapper::finalize_static)
#ifdef LOG_ACTION
                         .def("show", &CScriptActionPlannerAction::show)
#endif
                         .def("weight", &CScriptActionPlannerAction::weight,
                             &CScriptActionPlannerActionWrapper::weight_static)];
}

SCRIPT_EXPORT_FUNC(
    CScriptActionPlannerAction, (CScriptActionPlanner, CScriptActionBase), CScriptActionPlannerAction_Export);
