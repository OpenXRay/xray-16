#include "WeaponAutomaticShotgun.h"
#include "WeaponShotgun.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponShotgun, (CGameObject), {
    module(luaState)[class_<CWeaponShotgun, CGameObject>("CWeaponShotgun").def(constructor<>()),
        class_<CWeaponAutomaticShotgun, CGameObject>("CWeaponAutomaticShotgun").def(constructor<>())];
});
