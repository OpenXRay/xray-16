#include "pch_script.h"
#include "weaponbinoculars.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponBinoculars, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponBinoculars, CWeaponMagazined>("CWeaponBinoculars").def(constructor<>())]; });
