#include "pch_script.h"
#include "weaponvintorez.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponVintorez::CWeaponVintorez(void) : CWeaponMagazined(SOUND_TYPE_WEAPON_SNIPERRIFLE)
{}

CWeaponVintorez::~CWeaponVintorez(void)
{}

using namespace luabind;

SCRIPT_EXPORT(CWeaponVintorez, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponVintorez,CGameObject>("CWeaponVintorez")
			.def(constructor<>())
	];
});
