#include "pch_script.h"
#include "boar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CAI_Boar, (CGameObject),
{
	module(luaState)
	[
		class_<CAI_Boar,CGameObject>("CAI_Boar")
			.def(constructor<>())
	];
});
