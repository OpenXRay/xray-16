#include "pch_script.h"
#include "pseudo_gigant.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CPseudoGigant, (CGameObject),
{
	module(luaState)
	[
		class_<CPseudoGigant,CGameObject>("CPseudoGigant")
			.def(constructor<>())
	];
});
