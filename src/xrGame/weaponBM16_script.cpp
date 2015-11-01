#include "pch_script.h"
#include "WeaponBM16.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponBM16, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponBM16,CGameObject>("CWeaponBM16")
			.def(constructor<>())
	];
});
