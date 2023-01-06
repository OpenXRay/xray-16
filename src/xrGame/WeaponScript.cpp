#include "pch_script.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponBinoculars.h"
#include "weaponBM16.h"
#include "F1.h"
#include "WeaponFN2000.h"
#include "WeaponFORT.h"
#include "WeaponAmmo.h"
#include "WeaponHPSA.h"
#include "WeaponKnife.h"
#include "WeaponLR300.h"
#include "WeaponPM.h"
#include "RGD5.h"
#include "WeaponRPG7.h"
#include "WeaponSVD.h"
#include "WeaponSVU.h"
#include "WeaponAK74.h"
#include "WeaponAutomaticShotgun.h"
#include "WeaponGroza.h"
#include "WeaponRG6.h"
#include "WeaponShotgun.h"
#include "WeaponUSP45.h"
#include "WeaponVal.h"
#include "WeaponVintorez.h"
#include "WeaponWalther.h"
#include "medkit.h"
#include "antirad.h"
#include "FoodItem.h"
#include "BottleItem.h"
#include "ExplosiveItem.h"
#include "InventoryBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CWeapon, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeapon, CGameObject>("CWeapon")
            .def(constructor<>())
            .def("can_kill", (bool (CWeapon::*)() const)&CWeapon::can_kill)
    ];
});

SCRIPT_EXPORT(CWeaponMagazined, (CWeapon),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponMagazined, CWeapon>("CWeaponMagazined")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponMagazinedWGrenade, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponMagazinedWGrenade, CWeaponMagazined>("CWeaponMagazinedWGrenade")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponBinoculars, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponBinoculars, CWeaponMagazined>("CWeaponBinoculars")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponBM16, (CWeaponShotgun),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponBM16, CWeaponShotgun>("CWeaponBM16")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponFN2000, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponFN2000, CWeaponMagazined>("CWeaponFN2000")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponFORT, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponFORT, CWeaponMagazined>("CWeaponFORT")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CF1, (CGameObject, CExplosive),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CF1, bases<CGameObject, CExplosive>>("CF1")
            .def(constructor<>()),
        // new 14.10.08 peacemaker
        class_<CWeaponAmmo, CGameObject>("CWeaponAmmo")
            .def(constructor<>()),
        class_<CMedkit, CGameObject>("CMedkit")
            .def(constructor<>()),
        class_<CAntirad, CGameObject>("CAntirad")
            .def(constructor<>()),
        class_<CFoodItem, CGameObject>("CFoodItem")
            .def(constructor<>()),
        class_<CBottleItem, CGameObject>("CBottleItem")
            .def(constructor<>()),
        class_<CInventoryBox, CGameObject>("CInventoryBox")
            .def(constructor<>()),
        class_<CExplosiveItem, bases<CGameObject, CExplosive>>("CExplosiveItem")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponHPSA, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponHPSA, CWeaponMagazined>("CWeaponHPSA")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponKnife, (CWeapon),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponKnife, CWeapon>("CWeaponKnife")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponLR300, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponLR300, CWeaponMagazined>("CWeaponLR300")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponPM, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponPM, CWeaponMagazined>("CWeaponPM")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CRGD5, (CGameObject, CExplosive),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CRGD5, bases<CGameObject, CExplosive>>("CRGD5")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponRPG7, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponRPG7, CWeaponMagazined>("CWeaponRPG7")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponSVD, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponSVD, CWeaponMagazined>("CWeaponSVD")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponSVU, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponSVU, CWeaponMagazined>("CWeaponSVU")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponAK74, (CWeaponMagazinedWGrenade),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponAK74, CWeaponMagazinedWGrenade>("CWeaponAK74")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponAutomaticShotgun, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponAutomaticShotgun, CWeaponMagazined>("CWeaponAutomaticShotgun")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponGroza, (CWeaponMagazinedWGrenade),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponGroza, CWeaponMagazinedWGrenade>("CWeaponGroza")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponRG6, (CWeaponShotgun),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponRG6, CWeaponShotgun>("CWeaponRG6")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponShotgun, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponShotgun, CWeaponMagazined>("CWeaponShotgun")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponUSP45, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponUSP45, CWeaponMagazined>("CWeaponUSP45")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponVal, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponVal, CWeaponMagazined>("CWeaponVal")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponVintorez, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponVintorez, CWeaponMagazined>("CWeaponVintorez")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CWeaponWalther, (CWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CWeaponWalther, CWeaponMagazined>("CWeaponWalther")
            .def(constructor<>())
    ];
});
