#include "pch_script.h"
#include "weaponvintorez.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponVintorez::CWeaponVintorez(void) : CWeaponMagazined(SOUND_TYPE_WEAPON_SNIPERRIFLE) {}
CWeaponVintorez::~CWeaponVintorez(void) {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponVintorez, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponVintorez, CWeaponMagazined>("CWeaponVintorez").def(constructor<>())]; });
