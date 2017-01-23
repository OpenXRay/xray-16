#include "pch_script.h"
#include "controller.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CController, (CGameObject),
{
	module(luaState)
	[
		class_<CController,CGameObject>("CController")
			.def(constructor<>())
	];
});
