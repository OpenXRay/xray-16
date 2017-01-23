////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_terrain_task_script.cpp
//	Created 	: 20.09.2005
//  Modified 	: 20.09.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart terrain task
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_smart_terrain_task.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CALifeSmartTerrainTask, (),
{
	module(luaState)
	[
		class_<CALifeSmartTerrainTask>("CALifeSmartTerrainTask")
			.def(					constructor<LPCSTR>())
			.def(					constructor<LPCSTR,u32>())
			.def(					constructor<GameGraph::_GRAPH_ID,u32>())
			.def("game_vertex_id",	&CALifeSmartTerrainTask::game_vertex_id)
			.def("level_vertex_id",	&CALifeSmartTerrainTask::level_vertex_id)
			.def("position",		&CALifeSmartTerrainTask::position)
	];
});
