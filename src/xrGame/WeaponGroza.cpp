#include "pch_script.h"
#include "weapongroza.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponGroza::CWeaponGroza() : CWeaponMagazinedWGrenade(SOUND_TYPE_WEAPON_SUBMACHINEGUN) {}
CWeaponGroza::~CWeaponGroza() {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponGroza, (CWeaponMagazinedWGrenade),
    { module(luaState)[class_<CWeaponGroza, CWeaponMagazinedWGrenade>("CWeaponGroza").def(constructor<>())]; });
