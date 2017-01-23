////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_ALife_Items_script.cpp
//  Created     : 19.09.2002
//  Modified    : 04.06.2003
//  Author      : Dmitriy Iassenev
//  Description : Server items for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSE_ALifeInventoryItem, (),
{
    module(luaState)
    [
        class_<CSE_ALifeInventoryItem>
            ("cse_alife_inventory_item")
//          .def(       constructor<LPCSTR>())
    ];
});

SCRIPT_EXPORT(CSE_ALifeItem, (CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem),
{
    module(luaState)
    [
        luabind_class_item2(
//      luabind_class_abstract2(
            CSE_ALifeItem,
            "cse_alife_item",
            CSE_ALifeDynamicObjectVisual,
            CSE_ALifeInventoryItem
        )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemTorch, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemTorch,
            "cse_alife_item_torch",
            CSE_ALifeItem
        )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemAmmo, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemAmmo,
            "cse_alife_item_ammo",
            CSE_ALifeItem
        )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeapon, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeapon,
            "cse_alife_item_weapon",
            CSE_ALifeItem
        )
        .def("clone_addons", &CSE_ALifeItemWeapon::clone_addons)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeaponShotGun, (CSE_ALifeItemWeapon),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeaponShotGun,
            "cse_alife_item_weapon_shotgun",
            CSE_ALifeItemWeapon
            )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeaponAutoShotGun, (CSE_ALifeItemWeapon),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeaponAutoShotGun,
            "cse_alife_item_weapon_auto_shotgun",
            CSE_ALifeItemWeapon
            )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemDetector, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemDetector,
            "cse_alife_item_detector",
            CSE_ALifeItem
        )
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemArtefact, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemArtefact,
            "cse_alife_item_artefact",
            CSE_ALifeItem
        )
    ];
});
