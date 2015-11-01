#include "pch_script.h"
#include "HairsZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CHairsZone, (CGameObject),
{
	module(luaState)
	[
		class_<CHairsZone,CGameObject>("CHairsZone")
			.def(constructor<>())
	];
});
