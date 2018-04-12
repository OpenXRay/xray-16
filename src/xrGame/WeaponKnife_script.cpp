#include "pch_script.h"
#include "WeaponKnife.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CWeaponKnife, (CWeapon),
    { module(luaState)[class_<CWeaponKnife, CWeapon>("CWeaponKnife").def(constructor<>())]; });
