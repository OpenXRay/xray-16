////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_brain_script.cpp
//	Created 	: 02.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster detail path manager class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_monster_detail_path_manager.h"
#include "alife_smart_terrain_task.h"

using namespace luabind;

#pragma optimize("s",on)
void CALifeMonsterDetailPathManager::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CALifeMonsterDetailPathManager>("CALifeMonsterDetailPathManager")
			.def("target",		(void (CALifeMonsterDetailPathManager::*)(const GameGraph::_GRAPH_ID &, const u32 &, const Fvector &))(&CALifeMonsterDetailPathManager::target))
			.def("target",		(void (CALifeMonsterDetailPathManager::*)(const GameGraph::_GRAPH_ID &))(&CALifeMonsterDetailPathManager::target))
			.def("target",		(void (CALifeMonsterDetailPathManager::*)(const CALifeSmartTerrainTask *))(&CALifeMonsterDetailPathManager::target))
			.def("speed	",		(void (CALifeMonsterDetailPathManager::*)(const float &))(&CALifeMonsterDetailPathManager::speed))
			.def("speed	",		(const float &(CALifeMonsterDetailPathManager::*)() const)(&CALifeMonsterDetailPathManager::speed))
			.def("completed",	&CALifeMonsterDetailPathManager::completed)
			.def("actual",		&CALifeMonsterDetailPathManager::actual)
			.def("failed",		&CALifeMonsterDetailPathManager::failed)
	];
}
