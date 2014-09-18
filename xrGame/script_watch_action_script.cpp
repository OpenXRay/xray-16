////////////////////////////////////////////////////////////////////////////
//	Module 		: script_watch_action_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script watch action class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_watch_action.h"
#include "script_game_object.h"
#include "sight_manager_space.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptWatchAction::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptWatchAction>("look")
			.enum_("look")
			[
				value("path_dir",				int(SightManager::eSightTypePathDirection)),
				value("search",					int(SightManager::eSightTypeSearch)),
				value("danger",					int(SightManager::eSightTypeCover)),
				value("point",					int(SightManager::eSightTypePosition)),
				value("fire_point",				int(SightManager::eSightTypeFirePosition)),
				value("cur_dir",				int(SightManager::eSightTypeCurrentDirection)),
				value("direction",				int(SightManager::eSightTypeDirection))
			]
			.def(								constructor<>())
			.def(								constructor<SightManager::ESightType>())
			.def(								constructor<SightManager::ESightType, Fvector &>())
			.def(								constructor<SightManager::ESightType, CScriptGameObject*>())
			.def(								constructor<SightManager::ESightType, CScriptGameObject*, LPCSTR>())

			// searchlight 
			.def(								constructor<const Fvector &,float,float>())
			.def(								constructor<CScriptGameObject*,float,float>())

			.def("object",						&CScriptWatchAction::SetWatchObject)		// time
			.def("direct",						&CScriptWatchAction::SetWatchDirection)		// time
			.def("type",						&CScriptWatchAction::SetWatchType)
			.def("bone",						&CScriptWatchAction::SetWatchBone)
			.def("completed",					(bool (CScriptWatchAction::*)())(&CScriptWatchAction::completed))
	];
}