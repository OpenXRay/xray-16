#include "pch_script.h"
#include "WeaponRPG7.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponRPG7, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponRPG7,CGameObject>("CWeaponRPG7")
			.def(constructor<>())
	];
});
