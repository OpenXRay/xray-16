#include "pch_script.h"
#include "WeaponAK74.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponAK74::CWeaponAK74(ESoundTypes eSoundType) : CWeaponMagazinedWGrenade(eSoundType) {}
CWeaponAK74::~CWeaponAK74() {}
using namespace luabind;

SCRIPT_EXPORT(CWeapon, (CGameObject),
{
    module(luaState)
    [
        class_<CWeapon, CGameObject>("CWeapon")
            .def(constructor<>())
            .def("can_kill", (bool (CWeapon::*)() const)&CWeapon::can_kill)
    ];
});

SCRIPT_EXPORT(CWeaponMagazined, (CWeapon),
    { module(luaState)[class_<CWeaponMagazined, CWeapon>("CWeaponMagazined").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponMagazinedWGrenade, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponMagazinedWGrenade, CWeaponMagazined>("CWeaponMagazinedWGrenade").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponAK74, (CWeaponMagazinedWGrenade),
    { module(luaState)[class_<CWeaponAK74, CWeaponMagazinedWGrenade>("CWeaponAK74").def(constructor<>())]; });
