#include "pch_script.h"
#include "fracture.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CFracture, (CGameObject),
{
	module(luaState)
	[
		class_<CFracture,CGameObject>("CFracture")
			.def(constructor<>())
	];
});
