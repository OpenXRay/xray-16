////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_patrol_path_manager_script.cpp
//	Created 	: 02.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster patrol path manager class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_monster_patrol_path_manager.h"

using namespace luabind;

Fvector CALifeMonsterPatrolPathManager__target_position	(CALifeMonsterPatrolPathManager *self)
{
	THROW	(self);
	return	(self->target_position());
}

#pragma optimize("s",on)
void CALifeMonsterPatrolPathManager::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CALifeMonsterPatrolPathManager>("CALifeMonsterPatrolPathManager")
			.def("path",					(void (CALifeMonsterPatrolPathManager::*)(LPCSTR))(&CALifeMonsterPatrolPathManager::path))
			.def("start_type",				(void (CALifeMonsterPatrolPathManager::*)(const EPatrolStartType	&))(&CALifeMonsterPatrolPathManager::start_type))
			.def("start_type",				(const EPatrolStartType	&(CALifeMonsterPatrolPathManager::*)() const)(&CALifeMonsterPatrolPathManager::start_type))
			.def("route_type",				(void (CALifeMonsterPatrolPathManager::*)(const EPatrolRouteType	&))(&CALifeMonsterPatrolPathManager::route_type))
			.def("route_type",				(const EPatrolRouteType	&(CALifeMonsterPatrolPathManager::*)() const)(&CALifeMonsterPatrolPathManager::route_type))
			.def("actual",					&CALifeMonsterPatrolPathManager::actual)
			.def("completed",				&CALifeMonsterPatrolPathManager::completed)
			.def("start_vertex_index",		&CALifeMonsterPatrolPathManager::start_vertex_index)
			.def("use_randomness",			(void (CALifeMonsterPatrolPathManager::*)(const bool &))(&CALifeMonsterPatrolPathManager::use_randomness))
			.def("use_randomness",			(bool (CALifeMonsterPatrolPathManager::*)() const)(&CALifeMonsterPatrolPathManager::use_randomness))
			.def("target_game_vertex_id",	&CALifeMonsterPatrolPathManager::target_game_vertex_id)
			.def("target_level_vertex_id",	&CALifeMonsterPatrolPathManager::target_level_vertex_id)
			.def("target_position",			&CALifeMonsterPatrolPathManager__target_position)
	];
}
