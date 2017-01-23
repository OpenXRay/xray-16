#include "WeaponHPSA.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponHPSA, (CGameObject),
    { module(luaState)[class_<CWeaponHPSA, CGameObject>("CWeaponHPSA").def(constructor<>())]; });
