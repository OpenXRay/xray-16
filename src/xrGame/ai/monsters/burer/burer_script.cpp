#include "pch_script.h"
#include "burer.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CBurer, (CGameObject),
{
	module(luaState)
	[
		class_<CBurer,CGameObject>("CBurer")
			.def(constructor<>())
	];
});
