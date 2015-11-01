////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_crow_script.cpp
//	Created 	: 24.12.2007
//  Modified 	: 24.12.2007
//	Author		: Alexander Dudin
//	Description : Crow script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai/crow/ai_crow.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CAI_Crow, (CGameObject),
{
	module(luaState)
	[
		class_<CAI_Crow,CGameObject>("CAI_Crow")
			.def(constructor<>())
	];
});
