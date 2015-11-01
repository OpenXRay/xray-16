#include "pch_script.h"
#include "WeaponRG6.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponRG6, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponRG6,CGameObject>("CWeaponRG6")
			.def(constructor<>())
	];
});
