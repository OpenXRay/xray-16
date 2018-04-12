#include "pch_script.h"
#include "WeaponShotgun.h"
#include "WeaponAutomaticShotgun.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponShotgun, (CWeaponMagazined), {
    module(luaState)
    [
        class_<CWeaponShotgun, CWeaponMagazined>("CWeaponShotgun").def(constructor<>()),
        class_<CWeaponAutomaticShotgun, CWeaponMagazined>("CWeaponAutomaticShotgun").def(constructor<>())
    ];
});
