#include "pch_script.h"
#include "snork.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSnork, (CGameObject),
{
	module(luaState)
	[
		class_<CSnork,CGameObject>("CSnork")
			.def(constructor<>())
	];
});
