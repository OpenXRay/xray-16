#include "pch_script.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponBinoculars.h"
#include "WeaponBM16.h"
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
#include "Medkit.h"
#include "Antirad.h"
#include "FoodItem.h"
#include "BottleItem.h"
#include "ExplosiveItem.h"
#include "InventoryBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

SCRIPT_EXPORT(CWeapon, (CGameObject),
{
    module(luaState)
    [
        class_<CWeapon, CGameObject>("CWeapon")
            .def(constructor<>())
            .def("can_kill", (bool (CWeapon::*)() const) & CWeapon::can_kill)
            .def("ready_to_kill", (bool (CWeapon::*)() const) & CWeapon::ready_to_kill)
            .def("GetAddonsState", (u8(CWeapon::*)() const) & CWeapon::GetAddonsState)
            .def("SetAddonsState", &CWeapon::SetAddonsState)
            .def("IsZoomed", (bool (CWeapon::*)() const) & CWeapon::IsZoomed)
            .def("GetZoomFactor", (float (CWeapon::*)() const) & CWeapon::GetZoomFactor)
            .def("SetZoomFactor", &CWeapon::SetZoomFactor)
            .def("Weight", (float (CWeapon::*)() const) & CWeapon::Weight)
            .def("Cost", (u32(CWeapon::*)() const) & CWeapon::Cost)
            .def("GetMagazine", &CWeapon::GetMagazine, return_stl_iterator())
    ];
});

SCRIPT_EXPORT(CWeaponMagazined, (CWeapon),
    { module(luaState)[class_<CWeaponMagazined, CWeapon>("CWeaponMagazined").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponMagazinedWGrenade, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponMagazinedWGrenade, CWeaponMagazined>("CWeaponMagazinedWGrenade").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponBinoculars, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponBinoculars, CWeaponMagazined>("CWeaponBinoculars").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponBM16, (CWeaponShotgun),
    { module(luaState)[class_<CWeaponBM16, CWeaponShotgun>("CWeaponBM16").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponFN2000, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponFN2000, CWeaponMagazined>("CWeaponFN2000").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponFORT, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponFORT, CWeaponMagazined>("CWeaponFORT").def(constructor<>())]; });

SCRIPT_EXPORT(CF1, (CGameObject, CExplosive), {
    module(luaState)[class_<CF1, bases<CGameObject, CExplosive>>("CF1").def(constructor<>()),
    // new 14.10.08 peacemaker
    class_<CWeaponAmmo, CGameObject>("CWeaponAmmo")
            .def(constructor<>())
            .def_readwrite("m_boxSize", &CWeaponAmmo::m_boxSize)
            .def_readwrite("m_boxCurr", &CWeaponAmmo::m_boxCurr)
            .def_readwrite("m_tracer", &CWeaponAmmo::m_tracer)
            .def_readwrite("m_4to1_tracer", &CWeaponAmmo::m_4to1_tracer)
            .def("Weight", &CWeaponAmmo::Weight)
            .def("Cost", &CWeaponAmmo::Cost)
			.def("Get", &CWeaponAmmo::Get),
    class_<CMedkit, CGameObject>("CMedkit").def(constructor<>()),
    class_<CAntirad, CGameObject>("CAntirad").def(constructor<>()),
    class_<CFoodItem, CGameObject>("CFoodItem").def(constructor<>()),
    class_<CBottleItem, CGameObject>("CBottleItem").def(constructor<>()),
    class_<CInventoryBox, CGameObject>("CInventoryBox").def(constructor<>()),
    class_<CExplosiveItem, bases<CGameObject, CExplosive>>("CExplosiveItem").def(constructor<>())];
});

SCRIPT_EXPORT(CWeaponHPSA, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponHPSA, CWeaponMagazined>("CWeaponHPSA").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponKnife, (CWeapon),
    { module(luaState)[class_<CWeaponKnife, CWeapon>("CWeaponKnife").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponLR300, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponLR300, CWeaponMagazined>("CWeaponLR300").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponPM, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponPM, CWeaponMagazined>("CWeaponPM").def(constructor<>())]; });

SCRIPT_EXPORT(CRGD5, (CGameObject, CExplosive),
    { module(luaState)[class_<CRGD5, bases<CGameObject, CExplosive>>("CRGD5").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponRPG7, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponRPG7, CWeaponMagazined>("CWeaponRPG7").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponSVD, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponSVD, CWeaponMagazined>("CWeaponSVD").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponSVU, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponSVU, CWeaponMagazined>("CWeaponSVU").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponAK74, (CWeaponMagazinedWGrenade),
    { module(luaState)[class_<CWeaponAK74, CWeaponMagazinedWGrenade>("CWeaponAK74").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponAutomaticShotgun, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponAutomaticShotgun, CWeaponMagazined>("CWeaponAutomaticShotgun").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponGroza, (CWeaponMagazinedWGrenade),
    { module(luaState)[class_<CWeaponGroza, CWeaponMagazinedWGrenade>("CWeaponGroza").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponRG6, (CWeaponShotgun),
    { module(luaState)[class_<CWeaponRG6, CWeaponShotgun>("CWeaponRG6").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponShotgun, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponShotgun, CWeaponMagazined>("CWeaponShotgun").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponUSP45, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponUSP45, CWeaponMagazined>("CWeaponUSP45").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponVal, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponVal, CWeaponMagazined>("CWeaponVal").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponVintorez, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponVintorez, CWeaponMagazined>("CWeaponVintorez").def(constructor<>())]; });

SCRIPT_EXPORT(CWeaponWalther, (CWeaponMagazined),
    { module(luaState)[class_<CWeaponWalther, CWeaponMagazined>("CWeaponWalther").def(constructor<>())]; });

SCRIPT_EXPORT(CCartridge, (), {
    module(luaState)[class_<SCartridgeParam>("SCartridgeParam")
            .def(constructor<>())
            .def("Init", &SCartridgeParam::Init)
            .def_readwrite("kDist ", &SCartridgeParam::kDist)
            .def_readwrite("kDisp ", &SCartridgeParam::kDisp)
            .def_readwrite("kHit ", &SCartridgeParam::kHit)
            .def_readwrite("kImpulse ", &SCartridgeParam::kImpulse)
            .def_readwrite("kAP ", &SCartridgeParam::kAP)
            .def_readwrite("kAirRes ", &SCartridgeParam::kAirRes)
            .def_readwrite("kBulletSpeed ", &SCartridgeParam::kBulletSpeed)
            .def_readwrite("k_cam_dispersion", &SCartridgeParam::k_cam_dispersion)
            .def_readwrite("buckShot", &SCartridgeParam::buckShot)
            .def_readwrite("impair", &SCartridgeParam::impair)
            .def_readwrite("u8ColorID", &SCartridgeParam::u8ColorID),
        class_<CCartridge>("CCartridge")
            .def(constructor<>())
            .def("Weight", &CCartridge::Weight)
            .def_readwrite("m_LocalAmmoType", &CCartridge::m_LocalAmmoType)
            .def_readwrite("m_4to1_tracer", &CCartridge::m_4to1_tracer)
            .def_readwrite("bullet_material_idx", &CCartridge::bullet_material_idx)
            .def_readwrite("m_flags", &CCartridge::m_flags)
            .def_readwrite("param_s", &CCartridge::param_s)
            .enum_("cartridge_flags")[value("cfTracer", int(CCartridge::cfTracer)),
                value("cfRicochet", int(CCartridge::cfRicochet)),
                value("cfCanBeUnlimited", int(CCartridge::cfCanBeUnlimited)),
                value("cfExplosive", int(CCartridge::cfExplosive)),
                value("cfMagneticBeam", int(CCartridge::cfMagneticBeam))]
            .def("GetInventoryName", &CCartridge::GetInventoryName)];
});
