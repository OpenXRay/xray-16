////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_params_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path parameters class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "patrol_path_params.h"

using namespace luabind;

Fvector CPatrolPathParams__point(const CPatrolPathParams *self, u32 index)
{
	THROW	(self);
	return	(self->point(index));
}

#pragma optimize("s",on)
void CPatrolPathParams::script_register(lua_State *L)
{
	module(L)
	[
		class_<CPatrolPathParams>("patrol")
			.enum_("start")
			[
				value("start",					int(PatrolPathManager::ePatrolStartTypeFirst)),
				value("stop",					int(PatrolPathManager::ePatrolStartTypeLast)),
				value("nearest",				int(PatrolPathManager::ePatrolStartTypeNearest)),
				value("custom",					int(PatrolPathManager::ePatrolStartTypePoint)),
				value("next",					int(PatrolPathManager::ePatrolStartTypeNext)),
				value("dummy",					int(PatrolPathManager::ePatrolStartTypeDummy))
			]
			.enum_("stop")
			[
				value("stop",					int(PatrolPathManager::ePatrolRouteTypeStop)),
				value("continue",				int(PatrolPathManager::ePatrolRouteTypeContinue)),
				value("dummy",					int(PatrolPathManager::ePatrolRouteTypeDummy))
			]
			.def(								constructor<LPCSTR>())
			.def(								constructor<LPCSTR,const PatrolPathManager::EPatrolStartType>())
			.def(								constructor<LPCSTR,const PatrolPathManager::EPatrolStartType,const PatrolPathManager::EPatrolRouteType>())
			.def(								constructor<LPCSTR,const PatrolPathManager::EPatrolStartType,const PatrolPathManager::EPatrolRouteType, bool>())
			.def(								constructor<LPCSTR,const PatrolPathManager::EPatrolStartType,const PatrolPathManager::EPatrolRouteType, bool, u32>())
			.def("count",						&CPatrolPathParams::count)
			.def("level_vertex_id",				&CPatrolPathParams::level_vertex_id)
			.def("game_vertex_id",				&CPatrolPathParams::game_vertex_id)
			.def("point",						&CPatrolPathParams__point)
			.def("name",						&CPatrolPathParams::name)
			.def("index",						(u32			(CPatrolPathParams::*)(LPCSTR)			const)	(&CPatrolPathParams::point))
			.def("get_nearest",					(u32			(CPatrolPathParams::*)(const Fvector &) const)	(&CPatrolPathParams::point))
			.def("flag",						&CPatrolPathParams::flag)
			.def("flags",						&CPatrolPathParams::flags)
			.def("terminal",					&CPatrolPathParams::terminal)
	];
}
