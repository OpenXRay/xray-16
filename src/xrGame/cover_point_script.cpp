////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_point_script.cpp
//	Created 	: 24.03.2004
//  Modified 	: 24.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover point class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "cover_point.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

bool CCoverPoint__is_smart_cover	(CCoverPoint const* cover)
{
	return		(cover->m_is_smart_cover);
}

SCRIPT_EXPORT(CCoverPoint, (),
{
	module(luaState)
	[
		class_<CCoverPoint>("cover_point")
			.def("position",			&CCoverPoint::position)
			.def("level_vertex_id",		&CCoverPoint::level_vertex_id)
			.def("is_smart_cover",		&CCoverPoint__is_smart_cover)
	];
});
