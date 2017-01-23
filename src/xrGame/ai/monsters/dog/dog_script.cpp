#include "pch_script.h"
#include "dog.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CAI_Dog, (CGameObject),
{
	module(luaState)
	[
		class_<CAI_Dog,CGameObject>("CAI_Dog")
			.def(constructor<>())
	];
});
