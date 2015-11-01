////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_brain_script.cpp
//	Created 	: 02.11.2005
//  Modified 	: 02.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human brain class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_human_brain.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CALifeHumanBrain, (CALifeMonsterBrain),
{
	module(luaState)
	[
		class_<CALifeHumanBrain,CALifeMonsterBrain>("CALifeHumanBrain")
	];
});
