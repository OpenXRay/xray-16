#include "pch_script.h"
#include "weaponbinoculars.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponBinoculars, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponBinoculars,CGameObject>("CWeaponBinoculars")
			.def(constructor<>())
	];
});
