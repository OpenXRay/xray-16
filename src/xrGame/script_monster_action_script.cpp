////////////////////////////////////////////////////////////////////////////
//	Module 		: script_monster_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script monster action class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_monster_action.h"
#include "script_game_object.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptMonsterAction, (),
{
	module(luaState)
	[
		class_<CScriptMonsterAction>("act")
			.enum_("type")
			[
				value("rest",	int(MonsterSpace::eGA_Rest)),
				value("eat",	int(MonsterSpace::eGA_Eat)),
				value("attack",	int(MonsterSpace::eGA_Attack)),
				value("panic",	int(MonsterSpace::eGA_Panic))
			]

			.def(				constructor<>())
			.def(				constructor<MonsterSpace::EScriptMonsterGlobalAction>())
			.def(				constructor<MonsterSpace::EScriptMonsterGlobalAction, CScriptGameObject*>())
	];
});
