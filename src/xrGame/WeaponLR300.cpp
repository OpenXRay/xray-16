#include "pch_script.h"
#include "WeaponLR300.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponLR300::CWeaponLR300() : CWeaponMagazined(SOUND_TYPE_WEAPON_SUBMACHINEGUN) {}
CWeaponLR300::~CWeaponLR300() {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponLR300, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponLR300, CWeaponMagazined>("CWeaponLR300").def(constructor<>())]; });
