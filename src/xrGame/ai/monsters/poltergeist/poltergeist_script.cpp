#include "pch_script.h"
#include "poltergeist.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CPoltergeist, (CGameObject),
{
	module(luaState)
	[
		class_<CPoltergeist,CGameObject>("CPoltergeist")
			.def(constructor<>())
	];
});
