#include "pch_script.h"
#include "cat.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CCat, (CGameObject),
{
	module(luaState)
	[
		class_<CCat,CGameObject>("CCat")
			.def(constructor<>())
	];
});
