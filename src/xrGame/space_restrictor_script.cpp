#include "pch_script.h"
#include "space_restrictor.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSpaceRestrictor, (CGameObject),
{
	module(luaState)
	[
		class_<CSpaceRestrictor,CGameObject>("CSpaceRestrictor")
			.def(constructor<>())
	];
});
