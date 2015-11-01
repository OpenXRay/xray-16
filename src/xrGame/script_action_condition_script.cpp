////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_condition_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action condition class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_action_condition.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptActionCondition, (),
{
	module(luaState)
	[
		class_<CScriptActionCondition>("cond")
			.enum_("cond")
			[
				value("move_end",	int(CScriptActionCondition::MOVEMENT_FLAG)),
				value("look_end",	int(CScriptActionCondition::WATCH_FLAG)),
				value("anim_end",	int(CScriptActionCondition::ANIMATION_FLAG)),
				value("sound_end",	int(CScriptActionCondition::SOUND_FLAG)),
				value("object_end",	int(CScriptActionCondition::OBJECT_FLAG)),
				value("time_end",	int(CScriptActionCondition::TIME_FLAG)),
				value("act_end",	int(CScriptActionCondition::ACT_FLAG))
			]
			.def(					constructor<>())
			.def(					constructor<u32>())
			.def(					constructor<u32,double>())
	];
});
