#include "pch_script.h"
#include "flesh.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CAI_Flesh, (CGameObject),
{
	module(luaState)
	[
		class_<CAI_Flesh,CGameObject>("CAI_Flesh")
			.def(constructor<>())
	];
});
