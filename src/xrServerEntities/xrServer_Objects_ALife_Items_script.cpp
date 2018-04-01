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

void add_upgrade_script(CSE_ALifeInventoryItem* ta, pcstr str)
{
	ta->add_upgrade(str);
}

bool has_upgrade_script(CSE_ALifeInventoryItem* ta, pcstr str)
{
	return ta->has_upgrade(str);
}

using namespace luabind;

// clang-format off
SCRIPT_EXPORT(CSE_ALifeInventoryItem, (),
{
    module(luaState)
    [
        class_<CSE_ALifeInventoryItem>("cse_alife_inventory_item")
            //.def(constructor<pcstr>())
            .def("has_upgrade", &CSE_ALifeInventoryItem::has_upgrade)
            .def("add_upgrade", &CSE_ALifeInventoryItem::add_upgrade)
            .def_readwrite("condition", &CSE_ALifeInventoryItem::m_fCondition)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItem, (CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem),
{
    module(luaState)
    [
        luabind_class_item2(
        //luabind_class_abstract2(
        CSE_ALifeItem, "cse_alife_item", CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemTorch, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemTorch, "cse_alife_item_torch", CSE_ALifeItem)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemAmmo, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemAmmo, "cse_alife_item_ammo", CSE_ALifeItem)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeapon, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemWeapon, "cse_alife_item_weapon", CSE_ALifeItem)
            .enum_("addon_flag")
            [
                value("eWeaponAddonGrenadeLauncher", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonGrenadeLauncher)),
                value("eWeaponAddonScope", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonScope)),
                value("eWeaponAddonSilencer", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonSilencer)),
                value("eAddonAttachable", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonAttachable)),
                value("eAddonDisabled", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonDisabled)),
                value("eAddonPermanent", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonPermanent))
            ]
            .def("clone_addons", &CSE_ALifeItemWeapon::clone_addons)
            .def("get_ammo_elapsed", &CSE_ALifeItemWeapon::get_ammo_elapsed)
            .def("set_ammo_elapsed", &CSE_ALifeItemWeapon::set_ammo_elapsed)
            .def("get_ammo_magsize", &CSE_ALifeItemWeapon::get_ammo_magsize)
            .def_readwrite("grenade_launcher_status", &CSE_ALifeItemWeapon::m_grenade_launcher_status)
            .def_readwrite("silencer_status", &CSE_ALifeItemWeapon::m_silencer_status)
            .def_readwrite("scope_status", &CSE_ALifeItemWeapon::m_scope_status)
            .def_readwrite("ammo_type", &CSE_ALifeItemWeapon::ammo_type)
            .def("get_addon_flags", &CSE_ALifeItemWeapon::get_addon_flags)
            //.def("set_addon_flags", &CSE_ALifeItemWeapon::set_addon_flags)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeaponShotGun, (CSE_ALifeItemWeapon),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemWeaponShotGun, "cse_alife_item_weapon_shotgun", CSE_ALifeItemWeapon)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemWeaponAutoShotGun, (CSE_ALifeItemWeapon),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemWeaponAutoShotGun, "cse_alife_item_weapon_auto_shotgun", CSE_ALifeItemWeapon)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemDetector, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemDetector, "cse_alife_item_detector", CSE_ALifeItem)
    ];
});

SCRIPT_EXPORT(CSE_ALifeItemArtefact, (CSE_ALifeItem),
{
    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemArtefact, "cse_alife_item_artefact", CSE_ALifeItem)
    ];
});
// clang-format on
