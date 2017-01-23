#include "pch_script.h"
#include "WeaponKnife.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponKnife, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponKnife,CGameObject>("CWeaponKnife")
			.def(constructor<>())
	];
});
