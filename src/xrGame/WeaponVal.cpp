#include "pch_script.h"
#include "weaponval.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponVal::CWeaponVal(void) : CWeaponMagazined(SOUND_TYPE_WEAPON_SUBMACHINEGUN) {}
CWeaponVal::~CWeaponVal(void) {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponVal, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponVal, CWeaponMagazined>("CWeaponVal").def(constructor<>())]; });
