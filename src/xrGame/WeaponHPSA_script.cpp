#include "pch_script.h"
#include "WeaponHPSA.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponHPSA, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponHPSA, CWeaponMagazined>("CWeaponHPSA").def(constructor<>())]; });
