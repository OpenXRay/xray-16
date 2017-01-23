#include "pch_script.h"
#include "tushkano.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CTushkano, (CGameObject),
{
	module(luaState)
	[
		class_<CTushkano,CGameObject>("CTushkano")
		.def(constructor<>())
	];
});
