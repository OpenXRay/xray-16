#include "pch_script.h"
#include "GameTask.h"

using namespace luabind;

#pragma optimize("s",on)
void CGameTask::script_register(lua_State *L)
{
	module(L)
		[
			class_<enum_exporter<ETaskState> >("task")
				.enum_("task_state")
				[
					value("fail",					int(eTaskStateFail)),
					value("in_progress",			int(eTaskStateInProgress)),
					value("completed",				int(eTaskStateCompleted)),
					value("task_dummy",				int(eTaskStateDummy))
				]
				.enum_("task_type")
				[
					value("storyline",				int(eTaskTypeStoryline)),
					value("additional",				int(eTaskTypeAdditional))
				],


			class_<CGameTask>("CGameTask")
				.def(										constructor<>()									)
				.def("set_title",							&CGameTask::SetTitle_script						)
				.def("get_title",							&CGameTask::GetTitle_script						)
				.def("set_priority",						&CGameTask::SetPriority_script					)
				.def("get_priority",						&CGameTask::GetPriority_script					)
				.def("get_id",								&CGameTask::GetID_script						)
				.def("set_id",								&CGameTask::SetID_script						)
				.def("set_type",							&CGameTask::SetType_script						)
//				.def("get_type",							&CGameTask::GetType_script						)
				.def("set_icon_name",						&CGameTask::SetIconName_script					)
				.def("get_icon_name",						&CGameTask::GetIconName_script					)
				.def("set_description",						&CGameTask::SetDescription_script				)
				
				.def("set_map_hint",						&CGameTask::SetMapHint_script					)
				.def("set_map_location",					&CGameTask::SetMapLocation_script				)
				.def("set_map_object_id",					&CGameTask::SetMapObjectID_script				)

				.def("add_complete_info",					&CGameTask::AddCompleteInfo_script		)
				.def("add_fail_info",						&CGameTask::AddFailInfo_script			)
				.def("add_on_complete_info",				&CGameTask::AddOnCompleteInfo_script	)
				.def("add_on_fail_info",					&CGameTask::AddOnFailInfo_script		)
				
				.def("add_complete_func",					&CGameTask::AddCompleteFunc_script			)
				.def("add_fail_func",						&CGameTask::AddFailFunc_script				)
				.def("add_on_complete_func",				&CGameTask::AddOnCompleteFunc_script		)
				.def("add_on_fail_func",					&CGameTask::AddOnFailFunc_script			)

				.def("remove_map_locations",				&CGameTask::RemoveMapLocations			)
				.def("change_map_location",					&CGameTask::ChangeMapLocation			)
		];
}