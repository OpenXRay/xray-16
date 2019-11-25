#include "pch_script.h"
#include "GameTask.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CGameTask, (),
{
    class EnumTaskState {};

    module(luaState)
    [
        class_<EnumTaskState>("task")
            .enum_("task_state")
            [
                value("fail", int(eTaskStateFail)),
                value("in_progress", int(eTaskStateInProgress)),
                value("completed", int(eTaskStateCompleted)),
                value("task_dummy", int(eTaskStateDummy))
            ]
            .enum_("task_type")
            [
                value("storyline", int(eTaskTypeStoryline)),
                value("additional", int(eTaskTypeAdditional)),
                value("insignificant", int(eTaskTypeInsignificant))
            ],

        class_<SGameTaskObjective>("SGameTaskObjective")
            .def(constructor<CGameTask*, int>())
            .def("get_title", &SGameTaskObjective::GetTitle_script)
            .def("set_title", &SGameTaskObjective::SetTitle_script)
                
            .def("get_description", &SGameTaskObjective::GetDescription_script)
            .def("set_description", &SGameTaskObjective::SetDescription_script)

            .def("get_icon_name", &SGameTaskObjective::GetIconName_script)
            .def("set_icon_name", &SGameTaskObjective::SetIconName_script)

            .def("get_state", &SGameTaskObjective::GetTaskState)

            .def("get_type", &SGameTaskObjective::GetType_script)
            .def("set_type", &SGameTaskObjective::SetType_script)

            .def("set_map_hint", &SGameTaskObjective::SetMapHint_script)
            .def("set_map_location", &SGameTaskObjective::SetMapLocation_script)
            .def("set_map_object_id", &SGameTaskObjective::SetMapObjectID_script)

            .def("remove_map_locations", &SGameTaskObjective::RemoveMapLocations)
            .def("change_map_location", &SGameTaskObjective::ChangeMapLocation)

            .def("add_complete_info", &SGameTaskObjective::AddCompleteInfo_script)
            .def("add_complete_func", &SGameTaskObjective::AddCompleteFunc_script)

            .def("add_on_complete_info", &SGameTaskObjective::AddOnCompleteInfo_script)
            .def("add_on_complete_func", &SGameTaskObjective::AddOnCompleteFunc_script)

            .def("add_fail_info", &SGameTaskObjective::AddFailInfo_script)
            .def("add_fail_func", &SGameTaskObjective::AddFailFunc_script)

            .def("add_on_fail_info", &SGameTaskObjective::AddOnFailInfo_script)
            .def("add_on_fail_func", &SGameTaskObjective::AddOnFailFunc_script),

        class_<CGameTask, SGameTaskObjective>("CGameTask")
            .def(constructor<>())
            .def("get_id", &CGameTask::GetID_script)
            .def("set_id", &CGameTask::SetID_script)

            .def("get_priority", &CGameTask::GetPriority_script)
            .def("set_priority", &CGameTask::SetPriority_script)
    ];
});
