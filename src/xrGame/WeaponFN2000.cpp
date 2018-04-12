#include "pch_script.h"
#include "WeaponFN2000.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponFN2000::CWeaponFN2000() : CWeaponMagazined(SOUND_TYPE_WEAPON_SNIPERRIFLE) {}
CWeaponFN2000::~CWeaponFN2000() {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponFN2000, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponFN2000, CWeaponMagazined>("CWeaponFN2000").def(constructor<>())]; });
