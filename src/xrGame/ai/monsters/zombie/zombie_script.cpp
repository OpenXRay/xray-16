#include "pch_script.h"
#include "zombie.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CZombie, (CGameObject),
{
	module(luaState)
	[
		class_<CZombie,CGameObject>("CZombie")
			.def(constructor<>())
	];
});
