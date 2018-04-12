#include "pch_script.h"
#include "weaponusp45.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponUSP45::CWeaponUSP45() {}
CWeaponUSP45::~CWeaponUSP45() {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponUSP45, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponUSP45, CWeaponMagazined>("CWeaponUSP45").def(constructor<>())]; });
