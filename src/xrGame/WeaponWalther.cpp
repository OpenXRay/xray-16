#include "pch_script.h"
#include "weaponwalther.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponWalther::CWeaponWalther(void) {}
CWeaponWalther::~CWeaponWalther(void) {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponWalther, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponWalther, CWeaponMagazined>("CWeaponWalther").def(constructor<>())]; });
