#include "pch_script.h"
#include "WeaponRPG7.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponRPG7, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponRPG7, CWeaponMagazined>("CWeaponRPG7").def(constructor<>())]; });
